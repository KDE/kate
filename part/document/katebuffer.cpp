/* This file is part of the KDE libraries
   Copyright (c) 2000 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2002-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katebuffer.h"
#include "katebuffer.moc"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include "katedocument.h"
#include "katehighlight.h"
#include "kateconfig.h"
#include "kateglobal.h"
#include "kateautoindent.h"

#include <kdebug.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kencodingdetector.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtCore/QTextCodec>
#include <QtCore/QDate>

#include <limits.h>

/**
 * loader block size, load 256 kb at once per default
 * if file size is smaller, fall back to file size
 * must be a multiple of 2
 */
static const qint64 KATE_FILE_LOADER_BS  = 256 * 1024;

/**
 * hl will look at the next KATE_HL_LOOKAHEAD lines
 * or until the current block ends if a line is requested
 * will avoid to run doHighlight too often
 */
static const int KATE_HL_LOOKAHEAD = 64;

/**
 * Initial value for m_maxDynamicContexts
 */
static const int KATE_MAX_DYNAMIC_CONTEXTS = 512;

class KateFileLoader: private KEncodingDetector
{
  public:
    KateFileLoader (const QString &filename, QTextCodec *codec, bool removeTrailingSpaces, KEncodingDetector::AutoDetectScript script)
      : KEncodingDetector(codec,
                          script==KEncodingDetector::None?KEncodingDetector::UserChosenEncoding:KEncodingDetector::DefaultEncoding,
                          script)
      , m_eof (false) // default to not eof
      , m_lastWasEndOfLine (true) // at start of file, we had a virtual newline
      , m_lastWasR (false) // we have not found a \r as last char
      , m_binary (false)
      , m_removeTrailingSpaces (removeTrailingSpaces)
      , m_utf8Borked (false)
      , m_position (0)
      , m_lastLineStart (0)
      , m_eol (-1) // no eol type detected atm
      , m_file (filename)
      , m_buffer (qMin (m_file.size() == 0 ? KATE_FILE_LOADER_BS : m_file.size(), KATE_FILE_LOADER_BS), 0) // handle zero sized files special, like in /proc
    {
      kDebug (13020) << "OPEN USES ENCODING: " << codec->name();
    }

    ~KateFileLoader ()
    {
      //delete m_decoder;
    }

    /**
     * open file, read first chunk of data, detect eol (and possibly charset)
     */
    bool open ()
    {
      if (m_file.open (QIODevice::ReadOnly))
      {
        int c = m_file.read (m_buffer.data(), m_buffer.size());

        if (c > 0)
        {
          // fixes utf16 LE
          //may change codec if autodetection was set or BOM was found
          analyze(m_buffer.data(), c);
          m_utf8Borked=errorsIfUtf8(m_buffer.data(), c);
          m_binary=processNull(m_buffer.data(), c);
          m_text = decoder()->toUnicode(m_buffer, c);
        }

        m_eof = (c == -1) || (c == 0);

        for (int i=0; i < m_text.length(); i++)
        {
          if (m_text[i] == '\n')
          {
            m_eol = KateDocumentConfig::eolUnix;
            break;
          }
          else if ((m_text[i] == '\r'))
          {
            if (((i+1) < m_text.length()) && (m_text[i+1] == '\n'))
            {
              m_eol = KateDocumentConfig::eolDos;
              break;
            }
            else
            {
              m_eol = KateDocumentConfig::eolMac;
              break;
            }
          }
        }

        return true;
      }

      return false;
    }

    inline const char* actualEncoding () const { return encoding(); }

    // no new lines around ?
    inline bool eof () const { return m_eof && !m_lastWasEndOfLine && (m_lastLineStart == m_text.length()); }

    // eol mode ? autodetected on open(), -1 for no eol found in the first block!
    inline int eol () const { return m_eol; }

    // binary ?
    inline bool binary () const { return m_binary; }

    // broken utf8?
    inline bool brokenUTF8 () const { return m_utf8Borked; }

    // should spaces be ignored at end of line?
    inline bool removeTrailingSpaces () const { return m_removeTrailingSpaces; }

    // internal unicode data array
    inline const QChar *unicode () const { return m_text.unicode(); }

    // read a line, return length + offset in unicode data
    void readLine (int &offset, int &length)
    {
      length = 0;
      offset = 0;

      while (m_position <= m_text.length())
      {
        if (m_position == m_text.length())
        {
          // try to load more text if something is around
          if (!m_eof)
          {
            int c = m_file.read (m_buffer.data(), m_buffer.size());

            int readString = 0;
            if (c > 0)
            {
              m_binary=processNull(m_buffer.data(), c)||m_binary;
              m_utf8Borked=m_utf8Borked||errorsIfUtf8(m_buffer.data(), c);

              QString str (decoder()->toUnicode (m_buffer.data(), c));
              readString = str.length();

              m_text = m_text.mid (m_lastLineStart, m_position-m_lastLineStart)
                       + str;
            }
            else
              m_text = m_text.mid (m_lastLineStart, m_position-m_lastLineStart);

            // is file completely read ?
            m_eof = (c == -1) || (c == 0);

            // recalc current pos and last pos
            m_position -= m_lastLineStart;
            m_lastLineStart = 0;
          }

          // oh oh, end of file, escape !
          if (m_eof && (m_position == m_text.length()))
          {
            m_lastWasEndOfLine = false;

            // line data
            offset = m_lastLineStart;
            length = m_position-m_lastLineStart;

            m_lastLineStart = m_position;

            return;
          }
        }

        if (m_text[m_position] == '\n')
        {
          m_lastWasEndOfLine = true;

          if (m_lastWasR)
          {
            m_lastLineStart++;
            m_lastWasR = false;
          }
          else
          {
            // line data
            offset = m_lastLineStart;
            length = m_position-m_lastLineStart;

            m_lastLineStart = m_position+1;
            m_position++;

            return;
          }
        }
        else if (m_text[m_position] == '\r')
        {
          m_lastWasEndOfLine = true;
          m_lastWasR = true;

          // line data
          offset = m_lastLineStart;
          length = m_position-m_lastLineStart;

          m_lastLineStart = m_position+1;
          m_position++;

          return;
        }
        else
        {
          m_lastWasEndOfLine = false;
          m_lastWasR = false;
        }

        m_position++;
      }
    }


  private:
    bool m_eof;
    bool m_lastWasEndOfLine;
    bool m_lastWasR;
    bool m_binary;
    bool m_removeTrailingSpaces;
    bool m_utf8Borked;
    int m_position;
    int m_lastLineStart;
    int m_eol;
    QFile m_file;
    QByteArray m_buffer;
    QString m_text;
};

/**
 * Create an empty buffer. (with one block with one empty line)
 */
KateBuffer::KateBuffer(KateDocument *doc)
 : QObject (doc),
   editSessionNumber (0),
   editIsRunning (false),
   editTagLineStart (0xffffffff),
   editTagLineEnd (0),
   editTagLineFrom (false),
   editChangesDone (false),
   m_doc (doc),
   m_binary (false),
   m_brokenUTF8 (false),
   m_highlight (0),
   m_regionTree (this),
   m_tabWidth (8),
   m_lineHighlightedMax (0),
   m_lineHighlighted (0),
   m_maxDynamicContexts (KATE_MAX_DYNAMIC_CONTEXTS)
{
  clear();
}

/**
 * Cleanup on destruction
 */
KateBuffer::~KateBuffer()
{
  // release HL
  if (m_highlight)
    m_highlight->release();
}

void KateBuffer::editStart ()
{
  editSessionNumber++;

  if (editSessionNumber > 1)
    return;

  editIsRunning = true;

  editTagLineStart = INT_MAX;
  editTagLineEnd = 0;
  editTagLineFrom = false;

  editChangesDone = false;
}

void KateBuffer::editEnd ()
{
  if (editSessionNumber == 0)
    return;

  editSessionNumber--;

  if (editSessionNumber > 0)
    return;

  if (editChangesDone)
  {
    // hl update !!!
    if (m_highlight && editTagLineStart <= editTagLineEnd && editTagLineEnd <= m_lineHighlighted)
    {
      // look one line too far, needed for linecontinue stuff
      ++editTagLineEnd;

      // look one line before, needed nearly 100% only for indentation based folding !
      if (editTagLineStart > 0)
        --editTagLineStart;

      bool needContinue = doHighlight (
          editTagLineStart,
          editTagLineEnd,
          true);

      editTagLineStart = editTagLineEnd;

      if (needContinue)
        m_lineHighlighted = editTagLineStart;

      if (editTagLineStart > m_lineHighlightedMax)
        m_lineHighlightedMax = editTagLineStart;
    }
    else if (editTagLineStart < m_lineHighlightedMax)
      m_lineHighlightedMax = editTagLineStart;
  }

  editIsRunning = false;
}

void KateBuffer::clear()
{
  m_regionTree.clear();

  // one line
  m_lines.clear ();
  KateTextLine::Ptr textLine (new KateTextLine ());
  m_lines.push_back (textLine);

  // reset the state
  m_binary = false;
  m_brokenUTF8 = false;

  m_lineHighlightedMax = 0;
  m_lineHighlighted = 0;
}

bool KateBuffer::openFile (const QString &m_file)
{
   QTime t;
    t.start();

  KateFileLoader file (m_file, m_doc->config()->codec(), m_doc->config()->configFlags() & KateDocumentConfig::cfRemoveSpaces, m_doc->scriptForEncodingAutoDetection());

  bool ok = false;
  struct stat sbuf;
  if (stat(QFile::encodeName(m_file), &sbuf) == 0)
  {
    if (S_ISREG(sbuf.st_mode) && file.open())
      ok = true;
  }

  if (!ok)
  {
    clear();
    return false; // Error
  }

  m_doc->config()->setEncoding(file.actualEncoding());

  // set eol mode, if a eol char was found in the first 256kb block and we allow this at all!
  if (m_doc->config()->allowEolDetection() && (file.eol() != -1))
    m_doc->config()->setEol (file.eol());

  // flush current content, one line stays, therefor, remove that
  clear ();
  m_lines.clear ();

  // read in all lines...
  while ( !file.eof() )
  {
    int offset = 0, length = 0;
    file.readLine(offset, length);
    const QChar *unicodeData = file.unicode () + offset;

    // strip spaces at end of line
    if ( file.removeTrailingSpaces() )
    {
      while (length > 0)
      {
        if (unicodeData[length-1].isSpace())
          --length;
        else
          break;
      }
    }

    KateTextLine::Ptr textLine (new KateTextLine (unicodeData, length));
    m_lines.push_back (textLine);
  }

  // file was really empty, but we need ONE LINE!!!
  if (m_lines.isEmpty())
  {
    KateTextLine::Ptr textLine (new KateTextLine ());
    m_lines.push_back (textLine);
  }

  // fix region tree
  m_regionTree.fixRoot (m_lines.size());

  // binary?
  m_binary = file.binary ();

  // broken utf-8?
  m_brokenUTF8 = file.brokenUTF8();
  
  kDebug (13020) << "Broken UTF-8: " << m_brokenUTF8;

  kDebug (13020) << "LOADING DONE " << t.elapsed();

  return true;
}

bool KateBuffer::canEncode ()
{
  QTextCodec *codec = m_doc->config()->codec();

  kDebug(13020) << "ENC NAME: " << codec->name();

  // hardcode some unicode encodings which can encode all chars
  if ((QString(codec->name()) == "UTF-8") || (QString(codec->name()) == "ISO-10646-UCS-2"))
    return true;

  for (int i=0; i < m_lines.size(); i++)
  {
    if (!codec->canEncode (plainLine(i)->string()))
    {
      kDebug(13020) << "STRING LINE: " << plainLine(i)->string();
      kDebug(13020) << "ENC WORKING: FALSE";

      return false;
    }
  }

  return true;
}

bool KateBuffer::saveFile (const QString &m_file)
{
  QFile file (m_file);
  QTextStream stream (&file);

  if ( !file.open( QIODevice::WriteOnly ) )
  {
    return false; // Error
  }

  QTextCodec *codec = m_doc->config()->codec();

  // disable Unicode headers
  stream.setCodec(QTextCodec::codecForName("UTF-16"));

  // this line sets the mapper to the correct codec
  stream.setCodec(codec);

  // our loved eol string ;)
  QString eol = m_doc->config()->eolString ();

  // should we strip spaces?
  bool removeTrailingSpaces = m_doc->config()->configFlags() & KateDocumentConfig::cfRemoveSpaces;

  // just dump the lines out ;)
  for (int i=0; i < m_lines.size(); i++)
  {
    KateTextLine::Ptr textline = plainLine(i);

    // strip spaces
    if (removeTrailingSpaces)
    {
      int lastChar = textline->lastChar();

      if (lastChar > -1)
      {
        stream << textline->string().left(lastChar+1);
      }
    }
    else // simple, dump the line
      stream << textline->string();

    if ((i+1) < m_lines.size())
      stream << eol;
  }

  file.close ();

  return (file.error() == QFile::NoError);
}

KateTextLine::Ptr KateBuffer::line (int line)
{
  // valid line at all?
  if (line < 0 || line >= m_lines.size())
    return KateTextLine::Ptr();

  // already hl up-to-date for this line?
  if (line < m_lineHighlighted)
    return m_lines[line];

  // update hl until this line + max KATE_HL_LOOKAHEAD
  int end = qMin(line + KATE_HL_LOOKAHEAD, m_lines.size()-1);

  doHighlight ( m_lineHighlighted, end, false );

  m_lineHighlighted = end;

  // update hl max
  if (m_lineHighlighted > m_lineHighlightedMax)
    m_lineHighlightedMax = m_lineHighlighted;

  return m_lines[line];
}

void KateBuffer::changeLine(int i)
{
  if (i < 0 || i >= m_lines.size())
    return;

  // mark buffer changed
  editChangesDone = true;

  // tag this line as changed
  if (i < editTagLineStart)
    editTagLineStart = i;

  if (i > editTagLineEnd)
    editTagLineEnd = i;
}

void KateBuffer::insertLine(int i, KateTextLine::Ptr line)
{
  if (i < 0 || i > m_lines.size())
    return;

  m_lines.insert (i, line);

  if (m_lineHighlightedMax > i)
    m_lineHighlightedMax++;

  if (m_lineHighlighted > i)
    m_lineHighlighted++;

  // mark buffer changed
  editChangesDone = true;

  // tag this line as inserted
  if (i < editTagLineStart)
    editTagLineStart = i;

  if (i <= editTagLineEnd)
    editTagLineEnd++;

  if (i > editTagLineEnd)
    editTagLineEnd = i;

  // line inserted
  editTagLineFrom = true;

  m_regionTree.lineHasBeenInserted (i);
}

void KateBuffer::removeLine(int i)
{
  if (i < 0 || i >= m_lines.size())
    return;

  m_lines.remove (i);

  if (m_lineHighlightedMax > i)
    m_lineHighlightedMax--;

  if (m_lineHighlighted > i)
    m_lineHighlighted--;

  // mark buffer changed
  editChangesDone = true;

  // tag this line as removed
   if (i < editTagLineStart)
    editTagLineStart = i;

  if (i < editTagLineEnd)
    editTagLineEnd--;

  if (i > editTagLineEnd)
    editTagLineEnd = i;

  // make sure tags do not reach past the last line
  // see https://bugs.kde.org/show_bug.cgi?id=152497
  if (editTagLineEnd >= m_lines.size())
    editTagLineEnd = m_lines.size() - 1;

  if (editTagLineStart > editTagLineEnd)
    editTagLineStart = editTagLineEnd;

  // line removed
  editTagLineFrom = true;

  m_regionTree.lineHasBeenRemoved (i);
}

void KateBuffer::setTabWidth (int w)
{
  if ((m_tabWidth != w) && (m_tabWidth > 0))
  {
    m_tabWidth = w;

    if (m_highlight && m_highlight->foldingIndentationSensitive())
      invalidateHighlighting();
  }
}

void KateBuffer::setHighlight(int hlMode)
{
  KateHighlighting *h = KateHlManager::self()->getHl(hlMode);

   // aha, hl will change
  if (h != m_highlight)
  {
    bool invalidate = !h->noHighlighting();

    if (m_highlight)
    {
      m_highlight->release();
      invalidate = true;
    }

    h->use();

    // Clear code folding tree (see bug #124102)
    m_regionTree.clear();
    m_regionTree.fixRoot(m_lines.size());

    // try to set indentation
    if (!h->indentation().isEmpty())
      m_doc->config()->setIndentationMode (h->indentation());

    m_highlight = h;

    if (invalidate)
      invalidateHighlighting();

    // inform the document that the hl was really changed
    // needed to update attributes and more ;)
    m_doc->bufferHlChanged ();
  }
}

void KateBuffer::invalidateHighlighting()
{
  m_lineHighlightedMax = 0;
  m_lineHighlighted = 0;
}


void KateBuffer::updatePreviousNotEmptyLine(int current_line,bool addindent,int deindent)
{
  KateTextLine::Ptr textLine;
  do {
    if (current_line == 0) return;

    --current_line;

    textLine = m_lines[current_line];
  } while (textLine->firstChar()==-1);

  kDebug(13020)<<"updatePreviousNotEmptyLine: updating line:"<<current_line;
  QVector<int> foldingList=textLine->foldingListArray();
  while ( (foldingList.size()>0)  && ( abs(foldingList[foldingList.size()-2])==1)) {
    foldingList.resize(foldingList.size()-2);
  }
  addIndentBasedFoldingInformation(foldingList,textLine->length(),addindent,deindent);
  textLine->setFoldingList(foldingList);

  bool retVal_folding = false;
  m_regionTree.updateLine (current_line, &foldingList, &retVal_folding, true,false);

  emit tagLines (current_line, current_line);
}

void KateBuffer::addIndentBasedFoldingInformation(QVector<int> &foldingList,int linelength,bool addindent,int deindent)
{
  if (addindent) {
    //kDebug(13020)<<"adding indent for line :"<<current_line + buf->startLine()<<"  textLine->noIndentBasedFoldingAtStart"<<textLine->noIndentBasedFoldingAtStart();
    kDebug(13020)<<"adding ident";
    foldingList.resize (foldingList.size() + 2);
    foldingList[foldingList.size()-2] = 1;
    foldingList[foldingList.size()-1] = 0;
  }
  kDebug(13020)<<"DEINDENT: "<<deindent;
  if (deindent > 0)
  {
    //foldingList.resize (foldingList.size() + (deindent*2));

    //Make the whole last line marked as still belonging to the block
    for (int z=0;z<deindent;z++) {
      //FIXME: Not sure if this is really a performance problem
      foldingList.prepend(linelength+1);
      foldingList.prepend(-1);
    }

/*    for (int z= foldingList.size()-(deindent*2); z < foldingList.size(); z=z+2)
    {
      foldingList[z] = -1;
      foldingList[z+1] = 0;
    }*/
  }
}


bool KateBuffer::isEmptyLine(KateTextLine::Ptr textline)
{
  QLinkedList<QRegExp> l;
  l=m_highlight->emptyLines(textline->attribute(0));
  kDebug(13020)<<"trying to find empty line data";
  if (l.isEmpty()) return false;
  QString txt=textline->string();
  kDebug(13020)<<"checking empty line regexp";
  foreach(QRegExp re,l) {
    if (re.exactMatch(txt)) return true;
  }
  kDebug(13020)<<"no matches";
  return false;
}

bool KateBuffer::doHighlight (int startLine, int endLine, bool invalidate)
{
  // no hl around, no stuff to do
  if (!m_highlight)
    return false;

  /*if (m_highlight->foldingIndentationSensitive())
  {
    startLine=0;
    endLine=50;
  }*/

  //QTime t;
  //t.start();
  //kDebug (13020) << "HIGHLIGHTED START --- NEED HL, LINESTART: " << startLine << " LINEEND: " << endLine;
  //kDebug (13020) << "HL UNTIL LINE: " << m_lineHighlighted << " MAX: " << m_lineHighlightedMax;
  //kDebug (13020) << "HL DYN COUNT: " << KateHlManager::self()->countDynamicCtxs() << " MAX: " << m_maxDynamicContexts;

  // see if there are too many dynamic contexts; if yes, invalidate HL of all documents
  if (KateHlManager::self()->countDynamicCtxs() >= m_maxDynamicContexts)
  {
    {
      if (KateHlManager::self()->resetDynamicCtxs())
      {
        kDebug (13020) << "HL invalidated - too many dynamic contexts ( >= " << m_maxDynamicContexts << ")";

        // avoid recursive invalidation
        KateHlManager::self()->setForceNoDCReset(true);

        for (int i=0; i < KateGlobal::self()->kateDocuments().size(); ++i)
          (KateGlobal::self()->kateDocuments())[i]->makeAttribs();

        // doHighlight *shall* do his work. After invalidation, some highlight has
        // been recalculated, but *maybe not* until endLine ! So we shall force it manually...
        doHighlight ( m_lineHighlighted, endLine, false );
        m_lineHighlighted = endLine;

        KateHlManager::self()->setForceNoDCReset(false);

        return false;
      }
      else
      {
        m_maxDynamicContexts *= 2;
        kDebug (13020) << "New dynamic contexts limit: " << m_maxDynamicContexts;
      }
    }
  }

  // get previous line, if any
  KateTextLine::Ptr prevLine;

  if (startLine >= 1)
    prevLine = m_lines[startLine-1];
  else
    prevLine = new KateTextLine ();

  // does we need to emit a signal for the folding changes ?
  bool codeFoldingUpdate = false;

  // here we are atm, start at start line in the block
  int current_line = startLine;

  // do we need to continue
  bool stillcontinue=false;
  bool indentContinueWhitespace=false;
  bool indentContinueNextWhitespace=false;
  // loop over the lines of the block, from startline to endline or end of block
  // if stillcontinue forces us to do so
  while ( (current_line < m_lines.size()) && (stillcontinue || (current_line <= endLine)) )
  {
    // current line
    KateTextLine::Ptr textLine = m_lines[current_line];

    QVector<int> foldingList;
    bool ctxChanged = false;

    m_highlight->doHighlight (prevLine.data(), textLine.data(), foldingList, ctxChanged);

    // debug stuff
    //kDebug () << "current line to hl: " << current_line + buf->startLine();
    //kDebug () << "text length: " << textLine->length() << " attribute list size: " << textLine->attributesList().size();
    /*
    const QVector<int> &ml (textLine->attributesList());
    for (int i=2; i < ml.size(); i+=3)
    {
      kDebug () << "start: " << ml[i-2] << " len: " << ml[i-1] << " at: " << ml[i] << " ";
    }
    kDebug ();
*/
    //
    // indentation sensitive folding
    //
    bool indentChanged = false;
    if (m_highlight->foldingIndentationSensitive())
    {
      // get the indentation array of the previous line to start with !
      QVector<unsigned short> indentDepth (prevLine->indentationDepthArray());

      // current indentation of this line
      int iDepth = textLine->indentDepth(m_tabWidth);
      if (current_line==0)
      {
          indentDepth.resize (1);
          indentDepth[0] = iDepth;
      }

      textLine->setNoIndentBasedFoldingAtStart(prevLine->noIndentBasedFolding());
      // this line is empty, beside spaces, or has indentaion based folding disabled, use indentation depth of the previous line !
      kDebug(13020)<<"current_line:"<<current_line<<" textLine->noIndentBasedFoldingAtStart"<<textLine->noIndentBasedFoldingAtStart();
      if ( (textLine->firstChar() == -1) || textLine->noIndentBasedFoldingAtStart() || isEmptyLine(textLine) )
      {
        // do this to get skipped empty lines indent right, which was given in the indenation array
        if (!prevLine->indentationDepthArray().isEmpty())
        {
          iDepth = (prevLine->indentationDepthArray())[prevLine->indentationDepthArray().size()-1];
          kDebug(13020)<<"reusing old depth as current";
        }
        else
        {
          iDepth = prevLine->indentDepth(m_tabWidth);
          kDebug(13020)<<"creating indentdepth for previous line";
        }
      }

      kDebug(13020)<<"iDepth:"<<iDepth;

      // query the next line indentation, if we are at the end of the block
      // use the first line of the next buf block
      int nextLineIndentation = 0;
      bool nextLineIndentationValid=true;
      indentContinueNextWhitespace=false;
      if ((current_line+1) < m_lines.size())
      {
        if ( (m_lines[current_line+1]->firstChar() == -1) || isEmptyLine(m_lines[current_line+1]) )
        {
          nextLineIndentation = iDepth;
          indentContinueNextWhitespace=true;
        }
        else
          nextLineIndentation = m_lines[current_line+1]->indentDepth(m_tabWidth);
      }
      else
      {
        nextLineIndentationValid=false;
      }

      if  (!textLine->noIndentBasedFoldingAtStart()) {

        if ((iDepth > 0) && (indentDepth.isEmpty() || (indentDepth[indentDepth.size()-1] < iDepth)))
        {
          kDebug(13020)<<"adding depth to \"stack\":"<<iDepth;
          indentDepth.append (iDepth);
        } else {
          if (!indentDepth.isEmpty())
          {
            for (int z=indentDepth.size()-1; z > -1; z--)
              if (indentDepth[z]>iDepth)
                indentDepth.resize(z);
            if ((iDepth > 0) && (indentDepth.isEmpty() || (indentDepth[indentDepth.size()-1] < iDepth)))
            {
              kDebug(13020)<<"adding depth to \"stack\":"<<iDepth;
              indentDepth.append (iDepth);
              if (prevLine->firstChar()==-1) {

              }
            }
          }
        }
      }

      if (!textLine->noIndentBasedFolding())
      {
        if (nextLineIndentationValid)
        {
          //if (textLine->firstChar()!=-1)
          {
            kDebug(13020)<<"nextLineIndentation:"<<nextLineIndentation;
            bool addindent=false;
            int deindent=0;
            if (!indentDepth.isEmpty())
              kDebug(13020)<<"indentDepth[indentDepth.size()-1]:"<<indentDepth[indentDepth.size()-1];
            if ((nextLineIndentation>0) && ( indentDepth.isEmpty() || (indentDepth[indentDepth.size()-1]<nextLineIndentation)))
            {
              kDebug(13020)<<"addindent==true";
              addindent=true;
            } else {
            if ((!indentDepth.isEmpty()) && (indentDepth[indentDepth.size()-1]>nextLineIndentation))
              {
                kDebug(13020)<<"....";
                for (int z=indentDepth.size()-1; z > -1; z--)
                {
                  kDebug(13020)<<indentDepth[z]<<"  "<<nextLineIndentation;
                  if (indentDepth[z]>nextLineIndentation)
                    deindent++;
                }
              }
            }
/*        }
        if (textLine->noIndentBasedFolding()) kDebug(13020)<<"=============================indentation based folding disabled======================";
        if (!textLine->noIndentBasedFolding()) {*/
            if ((textLine->firstChar()==-1)) {
              updatePreviousNotEmptyLine(current_line,addindent,deindent);
              codeFoldingUpdate=true;
            }
            else
            {
              addIndentBasedFoldingInformation(foldingList,textLine->length(),addindent,deindent);
            }
          }
        }
      }
      indentChanged = !(indentDepth == textLine->indentationDepthArray());

      // assign the new array to the textline !
      if (indentChanged)
        textLine->setIndentationDepth (indentDepth);

      indentContinueWhitespace=textLine->firstChar()==-1;
    }
    bool foldingColChanged=false;
    bool foldingChanged = false; //!(foldingList == textLine->foldingListArray());
    if (foldingList.size()!=textLine->foldingListArray().size()) {
      foldingChanged=true;
    } else {
      QVector<int>::ConstIterator it=foldingList.begin();
      QVector<int>::ConstIterator it1=textLine->foldingListArray().begin();
      bool markerType=true;
      for(;it!=foldingList.end();++it,++it1) {
        if  (markerType) {
          if ( ((*it)!=(*it1))) {
            foldingChanged=true;
            foldingColChanged=false;
            break;
          }
        } else {
            if ((*it)!=(*it1)) {
              foldingColChanged=true;
            }
        }
        markerType=!markerType;
      }
    }

    if (foldingChanged || foldingColChanged) {
      textLine->setFoldingList(foldingList);
      if (foldingChanged==false){
        textLine->setFoldingColumnsOutdated(textLine->foldingColumnsOutdated() | foldingColChanged);
      } else textLine->setFoldingColumnsOutdated(false);
    }
    bool retVal_folding = false;
    //perhaps make en enums out of the change flags
    m_regionTree.updateLine (current_line, &foldingList, &retVal_folding, foldingChanged,foldingColChanged);

    codeFoldingUpdate = codeFoldingUpdate | retVal_folding;

    // need we to continue ?
    stillcontinue = ctxChanged || indentChanged || indentContinueWhitespace || indentContinueNextWhitespace;

    // move around the lines
    prevLine = textLine;

    // increment line
    current_line++;
  }

  // tag the changed lines !
  if (invalidate)
    emit tagLines (startLine, current_line);

  // emit that we have changed the folding
  if (codeFoldingUpdate)
    emit codeFoldingUpdated();

  //kDebug (13020) << "HIGHLIGHTED END --- NEED HL, LINESTART: " << startLine << " LINEEND: " << endLine;
  //kDebug (13020) << "HL UNTIL LINE: " << m_lineHighlighted << " MAX: " << m_lineHighlightedMax;
  //kDebug (13020) << "HL DYN COUNT: " << KateHlManager::self()->countDynamicCtxs() << " MAX: " << m_maxDynamicContexts;
  //kDebug (13020) << "TIME TAKEN: " << t.elapsed();

  // if we are at the last line of the block + we still need to continue
  // return the need of that !
  return stillcontinue;
}

void KateBuffer::codeFoldingColumnUpdate(int lineNr) {
  KateTextLine::Ptr line=plainLine(lineNr);
  if (!line) return;
  if (line->foldingColumnsOutdated()) {
    line->setFoldingColumnsOutdated(false);
    bool tmp;
    QVector<int> folding=line->foldingListArray();
    m_regionTree.updateLine(lineNr,&folding,&tmp,true,false);
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
