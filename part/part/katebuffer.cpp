/* This file is part of the KDE libraries
   Copyright (c) 2000 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2002-2004 Christoph Cullmann <cullmann@kde.org>

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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include "katebuffer.h"
#include "katebuffer.moc"

#include "katedocument.h"
#include "katehighlight.h"
#include "kateconfig.h"
#include "kateglobal.h"
#include "kateautoindent.h"

#include <kdebug.h>
#include <kglobal.h>
#include <kcharsets.h>

#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QTextCodec>
#include <QDateTime>

#include <limits.h>

/**
 * loader block size, load 256 kb at once per default
 * if file size is smaller, fall back to file size
 * must be a multiple of 2
 */
static const qint64 KATE_FILE_LOADER_BS  = 256 * 1024;

/**
 * KATE_AVG_BLOCK_SIZE is in characters !
 * (internaly we calc with approx 80 chars per line !)
 * block will max contain around BLOCK_SIZE chars or
 * BLOCK_LINES lines (after load, later that won't be tracked)
 */
static const int KATE_AVG_BLOCK_SIZE  = 2048 * 80;
static const int KATE_MAX_BLOCK_LINES = 2048;

/**
 * hl will look at the next KATE_HL_LOOKAHEAD lines
 * or until the current block ends if a line is requested
 * will avoid to run doHighlight too often
 */
static const int KATE_HL_LOOKAHEAD = 64;

/**
 * KATE_MAX_BLOCKS_LOADED should be at least 4, as some
 * methodes will cause heavy trashing, if not at least the
 * latest 2-3 used blocks are alive
 */
int KateBuffer::m_maxLoadedBlocks = 16;

/**
 * Initial value for m_maxDynamicContexts
 */
static const int KATE_MAX_DYNAMIC_CONTEXTS = 512;

void KateBuffer::setMaxLoadedBlocks (int count)
{
  m_maxLoadedBlocks = kMax ((int)4, count);
}

class KateFileLoader
{
  public:
    KateFileLoader (const QString &filename, QTextCodec *codec, bool removeTrailingSpaces)
      : m_file (filename)
      , m_buffer (kMin (m_file.size(), KATE_FILE_LOADER_BS))
      , m_codec (codec)
      , m_decoder (m_codec->makeDecoder())
      , m_position (0)
      , m_lastLineStart (0)
      , m_eof (false) // default to not eof
      , lastWasEndOfLine (true) // at start of file, we had a virtual newline
      , lastWasR (false) // we have not found a \r as last char
      , m_eol (-1) // no eol type detected atm
      , m_twoByteEncoding (QString(codec->name()) == "ISO-10646-UCS-2")
      , m_binary (false)
      , m_removeTrailingSpaces (removeTrailingSpaces)
    {
      kdDebug (13020) << "OPEN USES ENCODING: " << m_codec->name() << endl;
    }

    ~KateFileLoader ()
    {
      delete m_decoder;
    }

    /**
     * open file, read first chunk of data, detect eol
     */
    bool open ()
    {
      if (m_file.open (QIODevice::ReadOnly))
      {
        int c = m_file.readBlock (m_buffer.data(), m_buffer.size());

        if (c > 0)
        {
          // fix utf16 LE, stolen from khtml ;)
          if ((c >= 2) && (m_codec->mibEnum() == 1000) && (m_buffer[1] == '\0'))
          {
            // utf16LE, we need to put the decoder in LE mode
            char reverseUtf16[3] = {0xFF, 0xFE, 0x00};
            m_decoder->toUnicode(reverseUtf16, 2);
          }

          processNull (c);
          m_text = m_decoder->toUnicode (m_buffer, c);
        }

        m_eof = (c == -1) || (c == 0) || (m_text.length() == 0) || m_file.atEnd();

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

    // no new lines around ?
    inline bool eof () const { return m_eof && !lastWasEndOfLine && (m_lastLineStart == m_text.length()); }

    // eol mode ? autodetected on open(), -1 for no eol found in the first block!
    inline int eol () const { return m_eol; }

    // binary ?
    inline bool binary () const { return m_binary; }

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
            int c = m_file.readBlock (m_buffer.data(), m_buffer.size());

            int readString = 0;
            if (c > 0)
            {
              processNull (c);

              QString str (m_decoder->toUnicode (m_buffer, c));
              readString = str.length();

              m_text = m_text.mid (m_lastLineStart, m_position-m_lastLineStart)
                       + str;
            }
            else
              m_text = m_text.mid (m_lastLineStart, m_position-m_lastLineStart);

            // is file completly read ?
            m_eof = (c == -1) || (c == 0) || (readString == 0) || m_file.atEnd();

            // recalc current pos and last pos
            m_position -= m_lastLineStart;
            m_lastLineStart = 0;
          }

          // oh oh, end of file, escape !
          if (m_eof && (m_position == m_text.length()))
          {
            lastWasEndOfLine = false;

            // line data
            offset = m_lastLineStart;
            length = m_position-m_lastLineStart;

            m_lastLineStart = m_position;

            return;
          }
        }

        if (m_text[m_position] == '\n')
        {
          lastWasEndOfLine = true;

          if (lastWasR)
          {
            m_lastLineStart++;
            lastWasR = false;
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
          lastWasEndOfLine = true;
          lastWasR = true;

          // line data
          offset = m_lastLineStart;
          length = m_position-m_lastLineStart;

          m_lastLineStart = m_position+1;
          m_position++;

          return;
        }
        else
        {
          lastWasEndOfLine = false;
          lastWasR = false;
        }

        m_position++;
      }
    }

    // this nice methode will kill all 0 bytes (or double bytes)
    // and remember if this was a binary or not ;)
    void processNull (int length)
    {
      if (m_twoByteEncoding)
      {
        for (int i=1; i < length; i+=2)
        {
          if ((m_buffer[i] == '\0') && (m_buffer[i-1] == '\0'))
          {
            m_binary = true;
            m_buffer[i] = ' ';
          }
        }
      }
      else
      {
        for (int i=0; i < length; i++)
        {
          if (m_buffer[i] == '\0')
          {
            m_binary = true;
            m_buffer[i] = ' ';
          }
        }
      }
    }

  private:
    QFile m_file;
    QByteArray m_buffer;
    QTextCodec *m_codec;
    QTextDecoder *m_decoder;
    QString m_text;
    int m_position;
    int m_lastLineStart;
    bool m_eof;
    bool lastWasEndOfLine;
    bool lastWasR;
    int m_eol;
    bool m_twoByteEncoding;
    bool m_binary;
    bool m_removeTrailingSpaces;
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
   m_lines (0),
   m_lastInSyncBlock (0),
   m_lastFoundBlock (0),
   m_cacheReadError(false),
   m_cacheWriteError(false),
   m_loadingBorked (false),
   m_binary (false),
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
  // DELETE ALL BLOCKS, will free mem
  for (int i=0; i < m_blocks.size(); i++)
    delete m_blocks[i];

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
    if ( m_highlight && !m_highlight->noHighlighting()
        && (editTagLineStart <= editTagLineEnd)
        && (editTagLineEnd <= m_lineHighlighted))
    {
      // look one line too far, needed for linecontinue stuff
      editTagLineEnd++;

      // look one line before, needed nearly 100% only for indentation based folding !
      if (editTagLineStart > 0)
        editTagLineStart--;

      KateBufBlock *buf2 = 0;
      bool needContinue = false;
      while ((buf2 = findBlock(editTagLineStart)))
      {
        needContinue = doHighlight (buf2,
          (editTagLineStart > buf2->startLine()) ? editTagLineStart : buf2->startLine(),
          (editTagLineEnd > buf2->endLine()) ? buf2->endLine() : editTagLineEnd,
          true);

        editTagLineStart = (editTagLineEnd > buf2->endLine()) ? buf2->endLine() : editTagLineEnd;

        if ((editTagLineStart >= m_lines) || (editTagLineStart >= editTagLineEnd))
          break;
      }

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

  // cleanup the blocks
  for (int i=0; i < m_blocks.size(); i++)
    delete m_blocks[i];

  m_blocks.clear ();

  // create a bufblock with one line, we need that, only in openFile we won't have that
  KateBufBlock *block = new KateBufBlock(this, 0, 0);
  m_blocks.append (block);

  // reset the state
  m_lines = block->lines();
  m_lastInSyncBlock = 0;
  m_lastFoundBlock = 0;
  m_cacheWriteError = false;
  m_cacheReadError = false;
  m_loadingBorked = false;
  m_binary = false;

  m_lineHighlightedMax = 0;
  m_lineHighlighted = 0;
}

bool KateBuffer::openFile (const QString &m_file)
{
  KateFileLoader file (m_file, m_doc->config()->codec(), m_doc->config()->configFlags() & KateDocumentConfig::cfRemoveSpaces);

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

  // set eol mode, if a eol char was found in the first 256kb block and we allow this at all!
  if (m_doc->config()->allowEolDetection() && (file.eol() != -1))
    m_doc->config()->setEol (file.eol());

  // flush current content
  clear ();

  // cleanup the blocks
  for (int i=0; i < m_blocks.size(); i++)
    delete m_blocks[i];

  m_blocks.clear ();

  // do the real work
  KateBufBlock *block = 0;
  m_lines = 0;
  while (!file.eof() && !m_cacheWriteError)
  {
    block = new KateBufBlock (this, block, 0, &file);

    m_lines = block->endLine ();

    if (m_cacheWriteError || (block->lines() == 0))
    {
      delete block;
      break;
    }
    else
      m_blocks.append (block);
  }

  // we had a cache write error, this load is really borked !
  if (m_cacheWriteError)
    m_loadingBorked = true;

  if (m_blocks.isEmpty() || (m_lines == 0))
  {
    // file was really empty, clean the buffers + emit the line changed
    // loadingBorked will be false for such files, not matter what happened
    // before
    clear ();
  }
  else
  {
    // fix region tree
    m_regionTree.fixRoot (m_lines);
  }

  // if we have no hl or the "None" hl activated, whole file is correct highlighted
  // after loading, which wonder ;)
  if (!m_highlight || m_highlight->noHighlighting())
  {
    m_lineHighlighted = m_lines;
    m_lineHighlightedMax = m_lines;
  }

  // binary?
  m_binary = file.binary ();

  kdDebug (13020) << "LOADING DONE" << endl;

  return !m_loadingBorked;
}

bool KateBuffer::canEncode ()
{
  QTextCodec *codec = m_doc->config()->codec();

  kdDebug(13020) << "ENC NAME: " << codec->name() << endl;

  // hardcode some unicode encodings which can encode all chars
  if ((QString(codec->name()) == "UTF-8") || (QString(codec->name()) == "ISO-10646-UCS-2"))
    return true;

  for (int i=0; i < m_lines; i++)
  {
    if (!codec->canEncode (plainLine(i)->string()))
    {
      kdDebug(13020) << "STRING LINE: " << plainLine(i)->string() << endl;
      kdDebug(13020) << "ENC WORKING: FALSE" << endl;

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
  stream.setEncoding(QTextStream::RawUnicode);

  // this line sets the mapper to the correct codec
  stream.setCodec(codec);

  // our loved eol string ;)
  QString eol = m_doc->config()->eolString ();

  // should we strip spaces?
  bool removeTrailingSpaces = m_doc->config()->configFlags() & KateDocumentConfig::cfRemoveSpaces;

  // just dump the lines out ;)
  for (int i=0; i < m_lines; i++)
  {
    KateTextLine::Ptr textline = plainLine(i);

    // strip spaces
    if (removeTrailingSpaces)
    {
      int lastChar = textline->lastChar();

      if (lastChar > -1)
      {
        stream << QConstString (textline->text(), lastChar+1).string();
      }
    }
    else // simple, dump the line
      stream << textline->string();

    if ((i+1) < m_lines)
      stream << eol;
  }

  file.close ();

  m_loadingBorked = false;

  return (file.error() == QFile::NoError);
}

KateTextLine::Ptr KateBuffer::line_internal (KateBufBlock *buf, int i)
{
  // update hl until this line + max KATE_HL_LOOKAHEAD
  KateBufBlock *buf2 = 0;
  while ((i >= m_lineHighlighted) && (buf2 = findBlock(m_lineHighlighted)))
  {
    int end = kMin(i + KATE_HL_LOOKAHEAD, buf2->endLine());

    doHighlight ( buf2,
                  kMax(m_lineHighlighted, buf2->startLine()),
                  end,
                  false );

    m_lineHighlighted = end;
  }

  // update hl max
  if (m_lineHighlighted > m_lineHighlightedMax)
    m_lineHighlightedMax = m_lineHighlighted;

  return buf->line (i - buf->startLine());
}

KateBufBlock *KateBuffer::findBlock_internal (int i, int *index)
{
  int lastLine = m_blocks[m_lastInSyncBlock]->endLine ();

  if (lastLine > i) // we are in a allready known area !
  {
    while (true)
    {
      KateBufBlock *buf = m_blocks[m_lastFoundBlock];

      if ( (buf->startLine() <= i)
           && (buf->endLine() > i) )
      {
        if (index)
          (*index) = m_lastFoundBlock;

        return m_blocks[m_lastFoundBlock];
      }

      if (i < buf->startLine())
        --m_lastFoundBlock;
      else
        ++m_lastFoundBlock;
    }
  }
  else // we need first to resync the startLines !
  {
    if ((m_lastInSyncBlock+1) < m_blocks.size())
      ++m_lastInSyncBlock;
    else
      return 0;

    for (; m_lastInSyncBlock < m_blocks.size(); ++m_lastInSyncBlock)
    {
      // get next block
      KateBufBlock *buf = m_blocks[m_lastInSyncBlock];

      // sync startLine !
      buf->setStartLine (lastLine);

      // is it allready the searched block ?
      if ((i >= lastLine) && (i < buf->endLine()))
      {
        // remember this block as last found !
        m_lastFoundBlock = m_lastInSyncBlock;

        if (index)
          (*index) = m_lastFoundBlock;

        return buf;
      }

      // increase lastLine with blocklinecount
      lastLine += buf->lines ();
    }
  }

  // no block found !
  // index will not be set to any useful value in this case !
  return 0;
}

void KateBuffer::changeLine(int i)
{
  KateBufBlock *buf = findBlock(i);

  if (!buf)
    return;

  // mark this block dirty
  buf->markDirty ();

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
  int index = 0;
  KateBufBlock *buf;
  if (i == m_lines)
    buf = findBlock(i-1, &index);
  else
    buf = findBlock(i, &index);

  if (!buf)
    return;

  buf->insertLine(i -  buf->startLine(), line);

  if (m_lineHighlightedMax > i)
    m_lineHighlightedMax++;

  if (m_lineHighlighted > i)
    m_lineHighlighted++;

  m_lines++;

  // last sync block adjust
  if (m_lastInSyncBlock > index)
    m_lastInSyncBlock = index;

  // last found
  if (m_lastInSyncBlock < m_lastFoundBlock)
    m_lastFoundBlock = m_lastInSyncBlock;

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
   int index = 0;
   KateBufBlock *buf = findBlock(i, &index);

   if (!buf)
     return;

  buf->removeLine(i -  buf->startLine());

  if (m_lineHighlightedMax > i)
    m_lineHighlightedMax--;

  if (m_lineHighlighted > i)
    m_lineHighlighted--;

  m_lines--;

  // trash away a empty block
  if (buf->lines() == 0)
  {
    // we need to change which block is last in sync
    if (m_lastInSyncBlock >= index)
    {
      m_lastInSyncBlock = index;

      if (buf->next())
      {
        if (buf->prev())
          buf->next()->setStartLine (buf->prev()->endLine());
        else
          buf->next()->setStartLine (0);
      }
    }

    // cu block !
    delete buf;
    m_blocks.erase (m_blocks.begin()+index);
  }
  else
  {
    // last sync block adjust
    if (m_lastInSyncBlock > index)
      m_lastInSyncBlock = index;
  }

  // last found
  if (m_lastInSyncBlock < m_lastFoundBlock)
    m_lastFoundBlock = m_lastInSyncBlock;

  // mark buffer changed
  editChangesDone = true;

  // tag this line as removed
   if (i < editTagLineStart)
    editTagLineStart = i;

  if (i < editTagLineEnd)
    editTagLineEnd--;

  if (i > editTagLineEnd)
    editTagLineEnd = i;

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

    // try to set indentation
    if (!h->indentation().isEmpty())
      m_doc->config()->setIndentationMode (KateAutoIndent::modeNumber(h->indentation()));

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


void KateBuffer::updatePreviousNotEmptyLine(KateBufBlock *blk,int current_line,bool addindent,int deindent)
{
  KateTextLine::Ptr textLine;
  do {
    if (current_line>0) current_line--;
    else
    {
      int line=blk->startLine()+current_line;
      if (line==0) return;
      line--;
      blk=findBlock(line);
      if (!blk) {
        kdDebug(13020)<<"updatePreviousNotEmptyLine: block not found, this must not happen"<<endl;
        return;
      }
      current_line=line-blk->startLine();
    }
    textLine = blk->line(current_line);
  } while (textLine->firstChar()==-1);
  kdDebug(13020)<<"updatePreviousNotEmptyLine: updating line:"<<(blk->startLine()+current_line)<<endl;
  QVector<int> foldingList=textLine->foldingListArray();
  while ( (foldingList.size()>0)  && ( abs(foldingList[foldingList.size()-2])==1)) {
    foldingList.resize(foldingList.size()-2);
  }
  addIndentBasedFoldingInformation(foldingList,addindent,deindent);
  textLine->setFoldingList(foldingList);
  bool retVal_folding = false;
  m_regionTree.updateLine (current_line + blk->startLine(), &foldingList, &retVal_folding, true,false);
  emit tagLines (blk->startLine()+current_line, blk->startLine()+current_line);
}

void KateBuffer::addIndentBasedFoldingInformation(QVector<int> &foldingList,bool addindent,int deindent)
{
  if (addindent) {
    //kdDebug(13020)<<"adding indent for line :"<<current_line + buf->startLine()<<"  textLine->noIndentBasedFoldingAtStart"<<textLine->noIndentBasedFoldingAtStart()<<endl;
    kdDebug(13020)<<"adding ident"<<endl;
    foldingList.resize (foldingList.size() + 2);
    foldingList[foldingList.size()-2] = 1;
    foldingList[foldingList.size()-1] = 0;
  }
  kdDebug(13020)<<"DEINDENT: "<<deindent<<endl;
  if (deindent > 0)
  {
    foldingList.resize (foldingList.size() + (deindent*2));

    for (int z= foldingList.size()-(deindent*2); z < foldingList.size(); z=z+2)
    {
      foldingList[z] = -1;
      foldingList[z+1] = 0;
    }
  }
}


bool KateBuffer::isEmptyLine(KateTextLine::Ptr textline)
{
  QLinkedList<QRegExp> l;
  l=m_highlight->emptyLines(textline->attribute(0));
  kdDebug(13020)<<"trying to find empty line data"<<endl;
  if (l.isEmpty()) return false;
  QString txt=textline->string();
  kdDebug(13020)<<"checking empty line regexp"<<endl;
  foreach(QRegExp re,l) {
    if (re.exactMatch(txt)) return true;
  }
  kdDebug(13020)<<"no matches"<<endl;
  return false;
}

bool KateBuffer::doHighlight (KateBufBlock *buf, int startLine, int endLine, bool invalidate)
{
  // no hl around, no stuff to do
  if (!m_highlight)
    return false;

  /*if (m_highlight->foldingIndentationSensitive())
  {
    startLine=0;
    endLine=50;
  }*/

  // we tried to start in a line behind this buf block !
  if (startLine >= (buf->startLine()+buf->lines()))
    return false;

  //QTime t;
  //t.start();
  //kdDebug (13020) << "HIGHLIGHTED START --- NEED HL, LINESTART: " << startLine << " LINEEND: " << endLine << endl;
  //kdDebug (13020) << "HL UNTIL LINE: " << m_lineHighlighted << " MAX: " << m_lineHighlightedMax << endl;
  //kdDebug (13020) << "HL DYN COUNT: " << KateHlManager::self()->countDynamicCtxs() << " MAX: " << m_maxDynamicContexts << endl;

  // see if there are too many dynamic contexts; if yes, invalidate HL of all documents
  if (KateHlManager::self()->countDynamicCtxs() >= m_maxDynamicContexts)
  {
    {
      if (KateHlManager::self()->resetDynamicCtxs())
      {
        kdDebug (13020) << "HL invalidated - too many dynamic contexts ( >= " << m_maxDynamicContexts << ")" << endl;

        // avoid recursive invalidation
        KateHlManager::self()->setForceNoDCReset(true);

        for (int i=0; i < KateGlobal::self()->kateDocuments().size(); ++i)
          (KateGlobal::self()->kateDocuments())[i]->makeAttribs();

        // doHighlight *shall* do his work. After invalidation, some highlight has
        // been recalculated, but *maybe not* until endLine ! So we shall force it manually...
        KateBufBlock *buf = 0;
        while ((endLine > m_lineHighlighted) && (buf = findBlock(m_lineHighlighted)))
        {
          int end = kMin(endLine, buf->endLine());

          doHighlight ( buf,
                        kMax(m_lineHighlighted, buf->startLine()),
                        end,
                        false );

          m_lineHighlighted = end;
        }

        KateHlManager::self()->setForceNoDCReset(false);

        return false;
      }
      else
      {
        m_maxDynamicContexts *= 2;
        kdDebug (13020) << "New dynamic contexts limit: " << m_maxDynamicContexts << endl;
      }
    }
  }

  // get the previous line, if we start at the beginning of this block
  // take the last line of the previous block
  KateTextLine::Ptr prevLine = 0;

  if ((startLine == buf->startLine()) && buf->prev() && (buf->prev()->lines() > 0))
    prevLine = buf->prev()->line (buf->prev()->lines() - 1);
  else if ((startLine > buf->startLine()) && (startLine <= buf->endLine()))
    prevLine = buf->line(startLine - buf->startLine() - 1);
  else
    prevLine = new KateTextLine ();

  // does we need to emit a signal for the folding changes ?
  bool codeFoldingUpdate = false;

  // here we are atm, start at start line in the block
  int current_line = startLine - buf->startLine();

  // do we need to continue
  bool stillcontinue=false;
  bool indentContinueWhitespace=false;
  bool indentContinueNextWhitespace=false;
  // loop over the lines of the block, from startline to endline or end of block
  // if stillcontinue forces us to do so
  while ( (current_line < buf->lines())
          && (stillcontinue || ((current_line + buf->startLine()) <= endLine)) )
  {
    // current line
    KateTextLine::Ptr textLine = buf->line(current_line);

    QVector<int> foldingList;
    bool ctxChanged = false;

    m_highlight->doHighlight (prevLine, textLine, &foldingList, &ctxChanged);

    // debug stuff
    //kdDebug () << "current line to hl: " << current_line + buf->startLine() << endl;
    //kdDebug () << "text length: " << textLine->length() << " attribute list size: " << textLine->attributesList().size() << endl;
    /*
    const QVector<int> &ml (textLine->attributesList());
    for (int i=2; i < ml.size(); i+=3)
    {
      kdDebug () << "start: " << ml[i-2] << " len: " << ml[i-1] << " at: " << ml[i] << " " << endl;
    }
    kdDebug () << endl;
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
      if ((current_line+buf->startLine())==0)
      {
          indentDepth.resize (1);
          indentDepth[0] = iDepth;
      }

      textLine->setNoIndentBasedFoldingAtStart(prevLine->noIndentBasedFolding());
      // this line is empty, beside spaces, or has indentaion based folding disabled, use indentation depth of the previous line !
      kdDebug(13020)<<"current_line:"<<current_line + buf->startLine()<<" textLine->noIndentBasedFoldingAtStart"<<textLine->noIndentBasedFoldingAtStart()<<endl;
      if ( (textLine->firstChar() == -1) || textLine->noIndentBasedFoldingAtStart() || isEmptyLine(textLine) )
      {
        // do this to get skipped empty lines indent right, which was given in the indenation array
        if (!prevLine->indentationDepthArray().isEmpty())
        {
          iDepth = (prevLine->indentationDepthArray())[prevLine->indentationDepthArray().size()-1];
          kdDebug(13020)<<"reusing old depth as current"<<endl;
        }
        else
        {
          iDepth = prevLine->indentDepth(m_tabWidth);
          kdDebug(13020)<<"creating indentdepth for previous line"<<endl;
        }
      }

      kdDebug(13020)<<"iDepth:"<<iDepth<<endl;

      // query the next line indentation, if we are at the end of the block
      // use the first line of the next buf block
      int nextLineIndentation = 0;
      bool nextLineIndentationValid=true;
      indentContinueNextWhitespace=false;
      if ((current_line+1) < buf->lines())
      {
        if ( (buf->line(current_line+1)->firstChar() == -1) || isEmptyLine(buf->line(current_line+1)) )
        {
          nextLineIndentation = iDepth;
          indentContinueNextWhitespace=true;
        }
        else
          nextLineIndentation = buf->line(current_line+1)->indentDepth(m_tabWidth);
      }
      else
      {
        KateBufBlock *blk = buf->next();

        if (blk && (blk->lines() > 0))
        {
          if ( (blk->line (0)->firstChar() == -1) )
          {
            nextLineIndentation = iDepth;
            indentContinueNextWhitespace=true;
          }
          else
            nextLineIndentation = blk->line (0)->indentDepth(m_tabWidth);
        }
        else nextLineIndentationValid=false;
      }

      if  (!textLine->noIndentBasedFoldingAtStart()) {

        if ((iDepth > 0) && (indentDepth.isEmpty() || (indentDepth[indentDepth.size()-1] < iDepth)))
        {
          kdDebug(13020)<<"adding depth to \"stack\":"<<iDepth<<endl;
          indentDepth.append (iDepth);
        } else {
          if (!indentDepth.isEmpty())
          {
            for (int z=indentDepth.size()-1; z > -1; z--)
              if (indentDepth[z]>iDepth)
                indentDepth.resize(z);
            if ((iDepth > 0) && (indentDepth.isEmpty() || (indentDepth[indentDepth.size()-1] < iDepth)))
            {
              kdDebug(13020)<<"adding depth to \"stack\":"<<iDepth<<endl;
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
            kdDebug(13020)<<"nextLineIndentation:"<<nextLineIndentation<<endl;
            bool addindent=false;
            int deindent=0;
            if (!indentDepth.isEmpty())
              kdDebug(13020)<<"indentDepth[indentDepth.size()-1]:"<<indentDepth[indentDepth.size()-1]<<endl;
            if ((nextLineIndentation>0) && ( indentDepth.isEmpty() || (indentDepth[indentDepth.size()-1]<nextLineIndentation)))
            {
              kdDebug(13020)<<"addindent==true"<<endl;
              addindent=true;
            } else {
            if ((!indentDepth.isEmpty()) && (indentDepth[indentDepth.size()-1]>nextLineIndentation))
              {
                kdDebug(13020)<<"...."<<endl;
                for (int z=indentDepth.size()-1; z > -1; z--)
                {
                  kdDebug(13020)<<indentDepth[z]<<"  "<<nextLineIndentation<<endl;
                  if (indentDepth[z]>nextLineIndentation)
                    deindent++;
                }
              }
            }
/*        }
        if (textLine->noIndentBasedFolding()) kdDebug(13020)<<"=============================indentation based folding disabled======================"<<endl;
        if (!textLine->noIndentBasedFolding()) {*/
            if ((textLine->firstChar()==-1)) {
              updatePreviousNotEmptyLine(buf,current_line,addindent,deindent);
              codeFoldingUpdate=true;
            }
            else
            {
              addIndentBasedFoldingInformation(foldingList,addindent,deindent);
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
    m_regionTree.updateLine (current_line + buf->startLine(), &foldingList, &retVal_folding, foldingChanged,foldingColChanged);

    codeFoldingUpdate = codeFoldingUpdate | retVal_folding;

    // need we to continue ?
    stillcontinue = ctxChanged || indentChanged || indentContinueWhitespace || indentContinueNextWhitespace;

    // move around the lines
    prevLine = textLine;

    // increment line
    current_line++;
  }

  buf->markDirty ();

  // tag the changed lines !
  if (invalidate)
    emit tagLines (startLine, current_line + buf->startLine());

  // emit that we have changed the folding
  if (codeFoldingUpdate)
    emit codeFoldingUpdated();

  //kdDebug (13020) << "HIGHLIGHTED END --- NEED HL, LINESTART: " << startLine << " LINEEND: " << endLine << endl;
  //kdDebug (13020) << "HL UNTIL LINE: " << m_lineHighlighted << " MAX: " << m_lineHighlightedMax << endl;
  //kdDebug (13020) << "HL DYN COUNT: " << KateHlManager::self()->countDynamicCtxs() << " MAX: " << m_maxDynamicContexts << endl;
  //kdDebug (13020) << "TIME TAKEN: " << t.elapsed() << endl;

  // if we are at the last line of the block + we still need to continue
  // return the need of that !
  return stillcontinue && ((current_line+1) == buf->lines());
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

//BEGIN KateBufBlock

KateBufBlock::KateBufBlock ( KateBuffer *parent, KateBufBlock *prev, KateBufBlock *next,
                             KateFileLoader *stream )
: m_state (KateBufBlock::stateDirty),
  m_startLine (0),
  m_lines (0),
  m_vmblock (0),
  m_vmblockSize (0),
  m_parent (parent),
  m_prev (prev),
  m_next (next),
  list (0),
  listPrev (0),
  listNext (0)
{
  // init startline + the next pointers of the neighbour blocks
  if (m_prev)
  {
    m_startLine = m_prev->endLine ();
    m_prev->m_next = this;
  }

  if (m_next)
    m_next->m_prev = this;

  // we have a stream, use it to fill the block !
  // this can lead to 0 line blocks which are invalid !
  if (stream)
  {
    // this we lead to either dirty or swapped state
    fillBlock (stream);
  }
  else // init the block if no stream given !
  {
    // fill in one empty line !
    KateTextLine::Ptr textLine = new KateTextLine ();
    m_stringList.push_back (textLine);
    m_lines++;

    // if we have allready enough blocks around, swap one
    if (m_parent->m_loadedBlocks.count() >= KateBuffer::maxLoadedBlocks())
      m_parent->m_loadedBlocks.first()->swapOut();

    // we are a new nearly empty dirty block
    m_state = KateBufBlock::stateDirty;
    m_parent->m_loadedBlocks.append (this);
  }
}

KateBufBlock::~KateBufBlock ()
{
  // sync prev/next pointers
  if (m_prev)
    m_prev->m_next = m_next;

  if (m_next)
    m_next->m_prev = m_prev;

  // if we have some swapped data allocated, free it now or never
  if (m_vmblock)
    KateGlobal::self()->vm()->free(m_vmblock);

  // remove me from the list I belong
  KateBufBlockList::remove (this);
}

void KateBufBlock::fillBlock (KateFileLoader *stream)
{
  // is allready too much stuff around in mem ?
  bool swap = m_parent->m_loadedBlocks.count() >= KateBuffer::maxLoadedBlocks();

  QByteArray rawData;

  // calcs the approx size for KATE_AVG_BLOCK_SIZE chars !
  if (swap)
    rawData.resize ((KATE_AVG_BLOCK_SIZE * sizeof(QChar)) + ((KATE_AVG_BLOCK_SIZE/80) * 8));

  char *buf = rawData.data ();
  int size = 0;
  int blockSize = 0;
  while (!stream->eof() && (blockSize < KATE_AVG_BLOCK_SIZE) && (m_lines < KATE_MAX_BLOCK_LINES))
  {
    int offset = 0, length = 0;
    stream->readLine(offset, length);
    const QChar *unicodeData = stream->unicode () + offset;

    // strip spaces at end of line
    if ( stream->removeTrailingSpaces() )
    {
      while (length > 0)
      {
        if (unicodeData[length-1].isSpace())
          --length;
        else
          break;
      }
    }

    blockSize += length;

    if (swap)
    {
      // create the swapped data on the fly, no need to waste time
      // via going over the textline classes and dump them !
      char attr = KateTextLine::flagNoOtherData;
      int pos = size;

      // calc new size
      size = size + 1 + sizeof(int) + (sizeof(QChar)*length);

      if (size > rawData.size ())
      {
        rawData.resize (size);
        buf = rawData.data ();
      }

      memcpy(buf+pos, (char *) &attr, 1);
      pos += 1;

      memcpy(buf+pos, (char *) &length, sizeof(int));
      pos += sizeof(int);

      memcpy(buf+pos, (char *) unicodeData, sizeof(QChar)*length);
      pos += sizeof(QChar)*length;
    }
    else
    {
      KateTextLine::Ptr textLine = new KateTextLine ();
      textLine->insertText (0, length, unicodeData);
      m_stringList.push_back (textLine);
    }

    m_lines++;
  }

  if (swap)
  {
    m_vmblock = KateGlobal::self()->vm()->allocate(size);
    m_vmblockSize = size;

    if (!rawData.isEmpty())
    {
      if (!KateGlobal::self()->vm()->copy(m_vmblock, rawData.data(), 0, size))
      {
        if (m_vmblock)
          KateGlobal::self()->vm()->free(m_vmblock);

        m_vmblock = 0;
        m_vmblockSize = 0;

        m_parent->m_cacheWriteError = true;
      }
    }

    // fine, we are swapped !
    m_state = KateBufBlock::stateSwapped;
  }
  else
  {
    // we are a new dirty block without any swap data
    m_state = KateBufBlock::stateDirty;
    m_parent->m_loadedBlocks.append (this);
  }

  kdDebug (13020) << "A BLOCK LOADED WITH LINES: " << m_lines << endl;
}

KateTextLine::Ptr KateBufBlock::line(int i)
{
  // take care that the string list is around !!!
  if (m_state == KateBufBlock::stateSwapped)
    swapIn ();

  // LRU
  if (!m_parent->m_loadedBlocks.isLast(this))
    m_parent->m_loadedBlocks.append (this);

  return m_stringList[i];
}

void KateBufBlock::insertLine(int i, KateTextLine::Ptr line)
{
  // take care that the string list is around !!!
  if (m_state == KateBufBlock::stateSwapped)
    swapIn ();

  m_stringList.insert (m_stringList.begin()+i, line);
  m_lines++;

  markDirty ();
}

void KateBufBlock::removeLine(int i)
{
  // take care that the string list is around !!!
  if (m_state == KateBufBlock::stateSwapped)
    swapIn ();

  m_stringList.erase (m_stringList.begin()+i);
  m_lines--;

  markDirty ();
}

void KateBufBlock::markDirty ()
{
  if (m_state != KateBufBlock::stateSwapped)
  {
    // LRU
    if (!m_parent->m_loadedBlocks.isLast(this))
      m_parent->m_loadedBlocks.append (this);

    if (m_state == KateBufBlock::stateClean)
    {
      // if we have some swapped data allocated which is dirty, free it now
      if (m_vmblock)
        KateGlobal::self()->vm()->free(m_vmblock);

      m_vmblock = 0;
      m_vmblockSize = 0;

      // we are dirty
      m_state = KateBufBlock::stateDirty;
    }
  }
}

void KateBufBlock::swapIn ()
{
  if (m_state != KateBufBlock::stateSwapped)
    return;

  QByteArray rawData (m_vmblockSize);

  // what to do if that fails ?
  if (!KateGlobal::self()->vm()->copy(rawData.data(), m_vmblock, 0, rawData.size()))
    m_parent->m_cacheReadError = true;

  // reserve mem, keep realloc away on push_back
  m_stringList.reserve (m_lines);

  char *buf = rawData.data();
  for (int i=0; i < m_lines; i++)
  {
    KateTextLine::Ptr textLine = new KateTextLine ();
    buf = textLine->restore (buf);
    m_stringList.push_back (textLine);
  }

  // if we have allready enough blocks around, swap one
  if (m_parent->m_loadedBlocks.count() >= KateBuffer::maxLoadedBlocks())
    m_parent->m_loadedBlocks.first()->swapOut();

  // fine, we are now clean again, save state + append to clean list
  m_state = KateBufBlock::stateClean;
  m_parent->m_loadedBlocks.append (this);
}

void KateBufBlock::swapOut ()
{
  if (m_state == KateBufBlock::stateSwapped)
    return;

  if (m_state == KateBufBlock::stateDirty)
  {
    bool haveHl = m_parent->m_highlight && !m_parent->m_highlight->noHighlighting();

    // Calculate size.
    int size = 0;
    for (int i=0; i < m_lines; i++)
      size += m_stringList[i]->dumpSize (haveHl);

    QByteArray rawData (size);
    char *buf = rawData.data();

    // Dump textlines
    for (int i=0; i < m_lines; i++)
      buf = m_stringList[i]->dump (buf, haveHl);

    m_vmblock = KateGlobal::self()->vm()->allocate(rawData.size());
    m_vmblockSize = rawData.size();

    if (!rawData.isEmpty())
    {
      if (!KateGlobal::self()->vm()->copy(m_vmblock, rawData.data(), 0, rawData.size()))
      {
        if (m_vmblock)
          KateGlobal::self()->vm()->free(m_vmblock);

        m_vmblock = 0;
        m_vmblockSize = 0;

        m_parent->m_cacheWriteError = true;

        return;
      }
    }
  }

  m_stringList.clear();

  // we are now swapped out, set state + remove us out of the lists !
  m_state = KateBufBlock::stateSwapped;
  KateBufBlockList::remove (this);
}

//END KateBufBlock

//BEGIN KateBufBlockList

KateBufBlockList::KateBufBlockList ()
 : m_count (0),
   m_first (0),
   m_last (0)
{
}

void KateBufBlockList::append (KateBufBlock *buf)
{
  if (buf->list)
    buf->list->removeInternal (buf);

  m_count++;

  // append a element
  if (m_last)
  {
    m_last->listNext = buf;

    buf->listPrev = m_last;
    buf->listNext = 0;

    m_last = buf;

    buf->list = this;

    return;
  }

  // insert the first element
  m_last = buf;
  m_first = buf;

  buf->listPrev = 0;
  buf->listNext = 0;

  buf->list = this;
}

void KateBufBlockList::removeInternal (KateBufBlock *buf)
{
  if (buf->list != this)
    return;

  m_count--;

  if ((buf == m_first) && (buf == m_last))
  {
    // last element removed !
    m_first = 0;
    m_last = 0;
  }
  else if (buf == m_first)
  {
    // first element removed
    m_first = buf->listNext;
    m_first->listPrev = 0;
  }
  else if (buf == m_last)
  {
    // last element removed
    m_last = buf->listPrev;
    m_last->listNext = 0;
  }
  else
  {
    buf->listPrev->listNext = buf->listNext;
    buf->listNext->listPrev = buf->listPrev;
  }

  buf->listPrev = 0;
  buf->listNext = 0;

  buf->list = 0;
}

//END KateBufBlockList

// kate: space-indent on; indent-width 2; replace-tabs on;
