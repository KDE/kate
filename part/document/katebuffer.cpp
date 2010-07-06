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
#include <kde_file.h>

// on the fly compression
#include <kfilterdev.h>
#include <kmimetype.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtCore/QTextCodec>
#include <QtCore/QDate>

#include <limits.h>

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

/**
 * Create an empty buffer. (with one block with one empty line)
 */
KateBuffer::KateBuffer(KateDocument *doc)
 : Kate::TextBuffer (doc),
   m_doc (doc),
   m_brokenEncoding (false),
   m_highlight (0),
   m_regionTree (this),
   m_tabWidth (8),
   m_lineHighlightedMax (0),
   m_lineHighlighted (0),
   m_maxDynamicContexts (KATE_MAX_DYNAMIC_CONTEXTS)
{
  // we need kate global to stay alive
  KateGlobal::incRef ();
}

/**
 * Cleanup on destruction
 */
KateBuffer::~KateBuffer()
{
  // release HL
  if (m_highlight)
    m_highlight->release();
  
  // release kate global
  KateGlobal::decRef ();
}

void KateBuffer::editStart ()
{
  if (!startEditing ())
    return;
}

void KateBuffer::editEnd ()
{
  if (!finishEditing())
    return;

  if (editingChangedBuffer ())
  {
    // hl update !!!
    if (m_highlight && editingMinimalLineChanged () <= editingMaximalLineChanged () && editingMaximalLineChanged () <= m_lineHighlighted)
    {
      // look one line too far, needed for linecontinue stuff
      int editTagLineEnd = editingMaximalLineChanged () + 1;
      int editTagLineStart = editingMinimalLineChanged ();

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
    else if (editingMinimalLineChanged () < m_lineHighlightedMax)
      m_lineHighlightedMax = editingMinimalLineChanged ();
  }
}

void KateBuffer::clear()
{
  // call original clear function
  Kate::TextBuffer::clear ();

  m_regionTree.clear();

  // reset the state
  m_brokenEncoding = false;

  m_lineHighlightedMax = 0;
  m_lineHighlighted = 0;
}

bool KateBuffer::openFile (const QString &m_file)
{
  // first: setup fallback and normal encoding
  setEncodingProberType (KateGlobalConfig::global()->proberType ());
  setFallbackTextCodec (KateGlobalConfig::global()->fallbackCodec ());
  setTextCodec (m_doc->config()->codec ());

  // setup eol
  setEndOfLineMode ((EndOfLineMode) m_doc->config()->eol());

  // remove trailing spaces?
  setRemoveTrailingSpaces (m_doc->config()->removeSpaces());

  // then, try to load the file
  m_brokenEncoding = false;
  if (!load (m_file, m_brokenEncoding))
    return false;

  // save back encoding
  m_doc->config()->setEncoding (textCodec()->name());

  // set eol mode, if a eol char was found
  if (m_doc->config()->allowEolDetection())
    m_doc->config()->setEol (endOfLineMode ());

  // generate a bom?
  if (generateByteOrderMark())
    m_doc->config()->setBom (true);

  // fix region tree
  m_regionTree.fixRoot (lines ());

  // okay, loading did work
  return true;
}

bool KateBuffer::canEncode ()
{
  QTextCodec *codec = m_doc->config()->codec();

  kDebug(13020) << "ENC NAME: " << codec->name();

  // hardcode some unicode encodings which can encode all chars
  if ((QString(codec->name()) == "UTF-8") || (QString(codec->name()) == "ISO-10646-UCS-2"))
    return true;

  for (int i=0; i < lines(); i++)
  {
    if (!codec->canEncode (line(i)->string()))
    {
      kDebug(13020) << "STRING LINE: " << line(i)->string();
      kDebug(13020) << "ENC WORKING: FALSE";

      return false;
    }
  }

  return true;
}

bool KateBuffer::saveFile (const QString &m_file)
{
  // first: setup fallback and normal encoding
  setEncodingProberType (KateGlobalConfig::global()->proberType ());
  setFallbackTextCodec (KateGlobalConfig::global()->fallbackCodec ());
  setTextCodec (m_doc->config()->codec ());

  // setup eol
  setEndOfLineMode ((EndOfLineMode) m_doc->config()->eol());

  // generate bom?
  setGenerateByteOrderMark (m_doc->config()->bom());

  // remove trailing spaces?
  setRemoveTrailingSpaces (m_doc->config()->removeSpaces());

  // try to save
  if (!save (m_file))
    return false;

  // no longer broken encoding, or we don't care
  m_brokenEncoding = false;

  // okay
  return true;
}

void KateBuffer::ensureHighlighted (int line)
{
  // valid line at all?
  if (line < 0 || line >= lines ())
    return;

  // already hl up-to-date for this line?
  if (line < m_lineHighlighted)
    return;

  // update hl until this line + max KATE_HL_LOOKAHEAD
  int end = qMin(line + KATE_HL_LOOKAHEAD, lines ()-1);

  doHighlight ( m_lineHighlighted, end, false );

  m_lineHighlighted = end;

  // update hl max
  if (m_lineHighlighted > m_lineHighlightedMax)
    m_lineHighlightedMax = m_lineHighlighted;
}

void KateBuffer::wrapLine (const KTextEditor::Cursor &position)
{
  // call original
  Kate::TextBuffer::wrapLine (position);

  if (m_lineHighlightedMax > position.line()+1)
    m_lineHighlightedMax++;

  if (m_lineHighlighted > position.line()+1)
    m_lineHighlighted++;

  m_regionTree.lineHasBeenInserted (position.line()+1);

}

void KateBuffer::unwrapLine (int line)
{
  // call original
  Kate::TextBuffer::unwrapLine (line);

  if (m_lineHighlightedMax > line)
    m_lineHighlightedMax--;

  if (m_lineHighlighted > line)
    m_lineHighlighted--;

  m_regionTree.lineHasBeenRemoved (line);
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
    m_regionTree.fixRoot(lines());

    m_highlight = h;

    if (invalidate)
      invalidateHighlighting();

    // inform the document that the hl was really changed
    // needed to update attributes and more ;)
    m_doc->bufferHlChanged ();

    // try to set indentation
    if (!h->indentation().isEmpty())
      m_doc->config()->setIndentationMode (h->indentation());
  }
}

void KateBuffer::invalidateHighlighting()
{
  m_lineHighlightedMax = 0;
  m_lineHighlighted = 0;
}


void KateBuffer::updatePreviousNotEmptyLine(int current_line,bool addindent,int deindent)
{
  Kate::TextLine textLine;
  do {
    if (current_line == 0) return;

    --current_line;

    textLine = plainLine (current_line);
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

  // tagLines() is emitted from KatBuffer::doHighlight()!
}

void KateBuffer::addIndentBasedFoldingInformation(QVector<int> &foldingList,int linelength,bool addindent,int deindent)
{
  if (addindent) {
    //kDebug(13020)<<"adding indent for line :"<<current_line + buf->startLine()<<"  textLine->noIndentBasedFoldingAtStart"<<textLine->noIndentBasedFoldingAtStart();
    //kDebug(13020)<<"adding ident";
    foldingList.resize (foldingList.size() + 2);
    foldingList[foldingList.size()-2] = 1;
    foldingList[foldingList.size()-1] = 0;
  }
  //kDebug(13020)<<"DEINDENT: "<<deindent;
  if (deindent > 0)
  {
    //foldingList.resize (foldingList.size() + (deindent*2));

    //Make the whole last line marked as still belonging to the block
    for (int z=0;z<deindent;z++) {
      foldingList << -1 << linelength+1;
    }

/*    for (int z= foldingList.size()-(deindent*2); z < foldingList.size(); z=z+2)
    {
      foldingList[z] = -1;
      foldingList[z+1] = 0;
    }*/
  }
}


bool KateBuffer::isEmptyLine(Kate::TextLine textline)
{
  QLinkedList<QRegExp> l;
  l=m_highlight->emptyLines(textline->attribute(0));
  if (l.isEmpty()) return false;
  QString txt=textline->string();
  foreach(const QRegExp &re,l) {
    if (re.exactMatch(txt)) return true;
  }
  return false;
}

bool KateBuffer::doHighlight (int startLine, int endLine, bool invalidate)
{
  // no hl around, no stuff to do
  if (!m_highlight)
    return false;

#ifdef BUFFER_DEBUGGING
  QTime t;
  t.start();
  kDebug (13020) << "HIGHLIGHTED START --- NEED HL, LINESTART: " << startLine << " LINEEND: " << endLine;
  kDebug (13020) << "HL UNTIL LINE: " << m_lineHighlighted << " MAX: " << m_lineHighlightedMax;
  kDebug (13020) << "HL DYN COUNT: " << KateHlManager::self()->countDynamicCtxs() << " MAX: " << m_maxDynamicContexts;
#endif

  // see if there are too many dynamic contexts; if yes, invalidate HL of all documents
  if (KateHlManager::self()->countDynamicCtxs() >= m_maxDynamicContexts)
  {
    {
      if (KateHlManager::self()->resetDynamicCtxs())
      {
#ifdef BUFFER_DEBUGGING
        kDebug (13020) << "HL invalidated - too many dynamic contexts ( >= " << m_maxDynamicContexts << ")";
#endif

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

#ifdef BUFFER_DEBUGGING
        kDebug (13020) << "New dynamic contexts limit: " << m_maxDynamicContexts;
#endif
      }
    }
  }

  // get previous line, if any
  Kate::TextLine prevLine;

  if (startLine >= 1)
    prevLine = plainLine (startLine-1);
  else
    prevLine = Kate::TextLine (new Kate::TextLineData ());

  // does we need to emit a signal for the folding changes ?
  bool codeFoldingUpdate = false;

  // here we are atm, start at start line in the block
  int current_line = startLine;
  int start_spellchecking = -1;
  int last_line_spellchecking = -1;
  // do we need to continue
  bool stillcontinue=false;
  bool indentContinueWhitespace=false;
  bool indentContinueNextWhitespace=false;
  // loop over the lines of the block, from startline to endline or end of block
  // if stillcontinue forces us to do so
  while ( (current_line < lines()) && (stillcontinue || (current_line <= endLine)) )
  {
    // current line
    Kate::TextLine textLine = plainLine (current_line);

    QVector<int> foldingList;
    bool ctxChanged = false;

    m_highlight->doHighlight (prevLine.data(), textLine.data(), foldingList, ctxChanged);

#ifdef BUFFER_DEBUGGING
    // debug stuff
    kDebug( 13020 ) << "current line to hl: " << current_line + buf->startLine();
    kDebug( 13020 ) << "text length: " << textLine->length() << " attribute list size: " << textLine->attributesList().size();

    const QVector<int> &ml (textLine->attributesList());
    for (int i=2; i < ml.size(); i+=3)
    {
      kDebug( 13020 ) << "start: " << ml[i-2] << " len: " << ml[i-1] << " at: " << ml[i] << " ";
    }
    kDebug( 13020 );
#endif

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

#ifdef BUFFER_DEBUGGING
      kDebug(13020)<<"current_line:"<<current_line<<" textLine->noIndentBasedFoldingAtStart"<<textLine->noIndentBasedFoldingAtStart();
#endif

      if ( (textLine->firstChar() == -1) || textLine->noIndentBasedFoldingAtStart() || isEmptyLine(textLine) )
      {
        // do this to get skipped empty lines indent right, which was given in the indenation array
        if (!prevLine->indentationDepthArray().isEmpty())
        {
          iDepth = (prevLine->indentationDepthArray())[prevLine->indentationDepthArray().size()-1];

#ifdef BUFFER_DEBUGGING
          kDebug(13020)<<"reusing old depth as current";
#endif
        }
        else
        {
          iDepth = prevLine->indentDepth(m_tabWidth);

#ifdef BUFFER_DEBUGGING
          kDebug(13020)<<"creating indentdepth for previous line";
#endif
        }
      }

#ifdef BUFFER_DEBUGGING
      kDebug(13020)<<"iDepth:"<<iDepth;
#endif

      // query the next line indentation, if we are at the end of the block
      // use the first line of the next buf block
      int nextLineIndentation = 0;
      bool nextLineIndentationValid=true;
      indentContinueNextWhitespace=false;
      if ((current_line+1) < lines())
      {
        if ( (plainLine (current_line+1)->firstChar() == -1) || isEmptyLine(plainLine (current_line+1)) )
        {
          nextLineIndentation = iDepth;
          indentContinueNextWhitespace=true;
        }
        else
          nextLineIndentation = plainLine (current_line+1)->indentDepth(m_tabWidth);
      }
      else
      {
        nextLineIndentationValid=false;
      }

      if  (!textLine->noIndentBasedFoldingAtStart()) {

        if ((iDepth > 0) && (indentDepth.isEmpty() || (indentDepth[indentDepth.size()-1] < iDepth)))
        {
#ifdef BUFFER_DEBUGGING
          kDebug(13020)<<"adding depth to \"stack\":"<<iDepth;
#endif

          indentDepth.append (iDepth);
        } else {
          if (!indentDepth.isEmpty())
          {
            for (int z=indentDepth.size()-1; z > -1; z--)
              if (indentDepth[z]>iDepth)
                indentDepth.resize(z);
            if ((iDepth > 0) && (indentDepth.isEmpty() || (indentDepth[indentDepth.size()-1] < iDepth)))
            {
#ifdef BUFFER_DEBUGGING
              kDebug(13020)<<"adding depth to \"stack\":"<<iDepth;
#endif

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
#ifdef BUFFER_DEBUGGING
            kDebug(13020)<<"nextLineIndentation:"<<nextLineIndentation;
#endif

            bool addindent=false;
            int deindent=0;

#ifdef BUFFER_DEBUGGING
            if (!indentDepth.isEmpty())
              kDebug(13020)<<"indentDepth[indentDepth.size()-1]:"<<indentDepth[indentDepth.size()-1];
#endif

            if ((nextLineIndentation>0) && ( indentDepth.isEmpty() || (indentDepth[indentDepth.size()-1]<nextLineIndentation)))
            {
#ifdef BUFFER_DEBUGGING
              kDebug(13020)<<"addindent==true";
#endif

              addindent=true;
            } else {
            if ((!indentDepth.isEmpty()) && (indentDepth[indentDepth.size()-1]>nextLineIndentation))
              {
#ifdef BUFFER_DEBUGGING
                kDebug(13020)<<"....";
#endif

                for (int z=indentDepth.size()-1; z > -1; z--)
                {
#ifdef BUFFER_DEBUGGING
                  kDebug(13020)<<indentDepth[z]<<"  "<<nextLineIndentation;
#endif

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
      QVector<int>::ConstIterator it=foldingList.constBegin();
      QVector<int>::ConstIterator it1=textLine->foldingListArray().constBegin();
      bool markerType=true;
      for(;it!=foldingList.constEnd();++it,++it1) {
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
    if (stillcontinue && start_spellchecking < 0) {
      start_spellchecking=current_line;
    }
    else if (!stillcontinue && start_spellchecking >= 0) {
      last_line_spellchecking=current_line;
    }
    // move around the lines
    prevLine = textLine;

    // increment line
    current_line++;
  }

  // tag the changed lines !
  if (invalidate) {
    emit tagLines (startLine, current_line);
    if(start_spellchecking >= 0 && lines() > 0) {
      emit respellCheckBlock(start_spellchecking,
                             qMin(lines()-1, (last_line_spellchecking==-1)?current_line:last_line_spellchecking));
    }
  }
  // emit that we have changed the folding
  if (codeFoldingUpdate)
    emit codeFoldingUpdated();

#ifdef BUFFER_DEBUGGING
  kDebug (13020) << "HIGHLIGHTED END --- NEED HL, LINESTART: " << startLine << " LINEEND: " << endLine;
  kDebug (13020) << "HL UNTIL LINE: " << m_lineHighlighted << " MAX: " << m_lineHighlightedMax;
  kDebug (13020) << "HL DYN COUNT: " << KateHlManager::self()->countDynamicCtxs() << " MAX: " << m_maxDynamicContexts;
  kDebug (13020) << "TIME TAKEN: " << t.elapsed();
#endif

  // if we are at the last line of the block + we still need to continue
  // return the need of that !
  return stillcontinue;
}

void KateBuffer::codeFoldingColumnUpdate(int lineNr) {
  Kate::TextLine line=plainLine(lineNr);
  if (!line) return;
  if (line->foldingColumnsOutdated()) {
    line->setFoldingColumnsOutdated(false);
    bool tmp;
    QVector<int> folding=line->foldingListArray();
    m_regionTree.updateLine(lineNr,&folding,&tmp,true,false);
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
