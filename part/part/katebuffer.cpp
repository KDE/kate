/* This file is part of the KDE libraries
   Copyright (c) 2000 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2002, 2003 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "katebuffer.h"
#include "katebuffer.moc"

#include "katedocument.h"
#include "katehighlight.h"
#include "kateconfig.h"

#include <kdebug.h>
#include <kglobal.h>
#include <kcharsets.h>

#include <qfile.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qtextcodec.h>

#include <assert.h>

/**
 * SOME LIMITS, may need testing which values are clever
 * AVG_BLOCK_SIZE is in bytes, the others are counts
 */
#define KATE_AVG_BLOCK_SIZE      32000
#define KATE_MAX_BLOCKS_LOADED   8
#define KATE_MAX_BLOCKS_CLEAN    4
#define KATE_MAX_BLOCKS_DIRTY    2

/**
 * The KateBufBlock class contains an amount of data representing
 * a certain number of lines.
 */
class KateBufBlock
{
  friend class KateBufBlockList;

  public:
    /**
     * Create an empty block. (empty == ONE line)
     */
    KateBufBlock (KateBuffer *parent, KateBufBlock *prev, KateBufBlock *next);
    
    /**
     * destroy this block and take care of freeing all mem
     */
    ~KateBufBlock ();
    
  public:
    /**
     * return line @p i
     * The first line of this block is line 0.
     */
    TextLine::Ptr line(uint i);
    
    /**
     * insert @p line in front of line @p i
     */
    void insertLine(uint i, TextLine::Ptr line);
    
    /**
     * remove line @p i
     */
    void removeLine(uint i);
    
    /**
     * mark this block as dirty, will invalidate the swap/raw data
     */
    void markDirty ();
    
  public:
    /**
     * fill the block with the lines from the given stream
     * lastCharEOL indicates if a trailing newline should be
     * appended at the end
     * internal there will be a limit for the taken lines per block
     * returns EOL as bool
     */
    bool fillBlock (QTextStream *stream, bool lastCharEOL);
  
  public:
    /**
     * first line in block
     */
    inline uint startLine () const { return m_startLine; };
    
    /**
     * update the first line, needed to keep it up to date
     */
    inline void setStartLine (uint line) { m_startLine = line; }
    
    /**
     * first line behind this block
     */
    inline uint endLine () const { return m_startLine + m_lines; }
    
    /**
     * lines in this block
     */
    inline uint lines () const { return m_lines; }
    
    /**
     * get indenation date
     */
    inline uint firstLineIndentation () { return m_firstLineIndentation; }
    inline bool firstLineOnlySpaces () { return m_firstLineOnlySpaces; }
    
    /**
     * get last line for highlighting
     */
    inline TextLine::Ptr lastLine () { return m_lastLine; }

    /**
     * need Highlight flag
     */
    inline bool needHighlight () const { return b_needHighlight; }
    inline void setNeedHighlight (bool hl) { b_needHighlight = hl; };
    
    /**
     * prev/next block
     */
    inline KateBufBlock *prev () { return m_prev; }
    inline KateBufBlock *next () { return m_next; }

  public:
    /**
     * state flags
     */
    enum State
    {
      stateSwapped = 1,
      stateLoaded = 2,
      stateClean = 3,
      stateDirty = 4
    };
    
    /**
     * returns the current state of this block
     */
    inline KateBufBlock::State state () const { return m_state; }
    
  private:
    /**
     * set the current state, will do some internal transitions
     */
    void setState (KateBufBlock::State state);
  
  /**
   * methodes to increase state
   */
  private:
    /**
     * swap in the kvmallocater data
     */
    void swapIn ();
  
    /**
     * create a valid stringlist
     */
    void buildStringList();

  /**
   * methodes to lower state
   */
  private:
    /**
     * Post Condition: b_vmDataValid is true, b_rawDataValid is false
     */
    void swapOut ();
    
    /**
     * Dispose of a stringList.
     * Post Condition: b_stringListValid is false.
     */
    void disposeStringList();
    
    /**
     * Copy stringlist back to raw data.
     * Post Condition: b_rawDataValid is true.
     */
    void flushStringList();
            
  private:
    /**
     * VERY IMPORTANT, state of this block
     * this uchar indicates if the block is swapped, loaded, clean or dirty
     */
    KateBufBlock::State m_state;
    
    /**
     * IMPORTANT, start line + lines in block
     */
    uint m_startLine;
    uint m_lines;

    /**
     * context & hlContinue flag + indentation infos
     * only used in the case that string list is not around
     */
    uint m_firstLineIndentation;
    bool m_firstLineOnlySpaces;
    TextLine::Ptr m_lastLine;

    /**
     * here we swap our stuff
     */
    KVMAllocator::Block *m_vmblock;
    uint m_vmblockSize;

    /**
     * raw data, if m_codec != 0 it is still the dump file content,
     * if m_codec == 0 it is the dumped string list
     */
    QByteArray m_rawData;

    /**
     * list of textlines
     */
    TextLine::List m_stringList;

    /**
     * buffer requires highlighting.
     */
    bool b_needHighlight;

    /**
     * parent buffer.
     */
    KateBuffer* m_parent;

    /**
     * prev/next block
     */
    KateBufBlock *m_prev;
    KateBufBlock *m_next;
    
  private:
    /**
     * list pointer, to which list I belong
     * list element pointers for the KateBufBlockList ONLY !!!
     */
    KateBufBlockList *list;
    KateBufBlock *listPrev;
    KateBufBlock *listNext;
};

/**
 * Create an empty buffer. (with one block with one empty line)
 */
KateBuffer::KateBuffer(KateDocument *doc)
 : QObject (doc),
   m_doc (doc),
   m_lines (0),
   m_lastInSyncBlock (0),
   m_lastFoundBlock (0),
   m_cacheReadError(false),
   m_cacheWriteError(false),
   m_loadingBorked (false),
   m_highlight (0),
   m_regionTree (this),
   m_highlightedTo (0),
   m_highlightedRequested (0),
   m_hlUpdate (true),
   m_tabWidth (8),
   m_highlightedTill (0),
   m_highlightedEnd (0),
   m_highlightedSteps (0)
{
  connect( &m_highlightTimer, SIGNAL(timeout()), this, SLOT(pleaseHighlight()));

  connect( &m_regionTree,SIGNAL(setLineVisible(unsigned int, bool)), this,SLOT(setLineVisible(unsigned int,bool)));
  
  clear();
}

/**
 * Cleanup on destruction
 */
KateBuffer::~KateBuffer()
{
  // DELETE ALL BLOCKS, will free mem
  for (uint i=0; i < m_blocks.size(); i++)
    delete m_blocks[i];
}

void KateBuffer::setTabWidth (uint w)
{
  if (m_tabWidth != w)
  {
    m_tabWidth = w;

    if (m_highlight && m_highlight->foldingIndentationSensitive())
      invalidateHighlighting();
  }
}

uint KateBuffer::countVisible ()
{
  return m_lines - m_regionTree.getHiddenLinesCount(m_lines);
}

uint KateBuffer::lineNumber (uint visibleLine)
{
  return m_regionTree.getRealLine (visibleLine);
}

uint KateBuffer::lineVisibleNumber (uint line)
{
  return m_regionTree.getVirtualLine (line);
}

void KateBuffer::lineInfo (KateLineInfo *info, unsigned int line)
{
  m_regionTree.getLineInfo(info,line);
}

KateCodeFoldingTree *KateBuffer::foldingTree ()
{
  return &m_regionTree;
}

/**
 * Find a block for line i + correct start/endlines for blocks
 */
KateBufBlock *KateBuffer::findBlock(uint i, uint *index)
{
  // out of range !
  if (i >= m_lines)
    return 0;

  uint lastLine = m_blocks[m_lastInSyncBlock]->endLine ();
    
  if (lastLine > i) // we are in a allready known area !
  {
    if ((m_lastFoundBlock >= m_blocks.size()) || (m_lastFoundBlock > m_lastInSyncBlock))
      m_lastFoundBlock = 0;
     
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
        m_lastFoundBlock--;
      else
        m_lastFoundBlock++;
    }
  }
  else // we need first to resync the startLines !
  {
    if ((m_lastInSyncBlock+1) < m_blocks.size())
      m_lastInSyncBlock++;
    else
      return 0;
  
    for (; m_lastInSyncBlock < m_blocks.size(); m_lastInSyncBlock++)
    {
      // get next block
      KateBufBlock *buf = m_blocks[m_lastInSyncBlock];
      
      // sync startLine !
      buf->setStartLine (lastLine);
      
      // is it allready the searched block ?
      if ((i >= lastLine) && (i < buf->endLine()))
      {
        if (index)
          (*index) = m_lastInSyncBlock;
      
        // remember this block as last found !
        m_lastFoundBlock = m_lastInSyncBlock;
        
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

void KateBuffer::clear()
{
  m_regionTree.clear();

  // cleanup the blocks  
  for (uint i=0; i < m_blocks.size(); i++)
    delete m_blocks[i];
    
  m_blocks.clear ();

  // create a bufblock with one line, we need that, only in openFile we won't have that
  KateBufBlock *block = new KateBufBlock(this, 0, 0);
  m_blocks.append (block);

  // reset the state
  m_lines = block->lines();
  m_highlightedTo = 0;
  m_highlightedRequested = 0;
  m_lastInSyncBlock = 0;
  m_lastFoundBlock = 0;
  m_cacheWriteError = false;
  m_cacheReadError = false;
  m_loadingBorked = false;

  emit linesChanged(m_lines);
}

void KateBuffer::setHighlight(Highlight *highlight)
{
  m_highlight = highlight;
  invalidateHighlighting();
}

/**
 * Insert a file at line @p line in the buffer.
 */
bool KateBuffer::openFile (const QString &m_file)
{
  QFile file (m_file);

  if ( !file.open( IO_ReadOnly ) || !file.isDirectAccess() )
  {
    clear();
    
    emit loadingFinished ();
    
    return false; // Error
  }

  // detect eol
  while (true)
  {
     int ch = file.getch();

     if (ch == -1)
       break;

     if ((ch == '\r'))
     {
       ch = file.getch ();

       if (ch == '\n')
       {
         m_doc->config()->setEol (KateDocumentConfig::eolDos);
         break;
       }
       else
       {
         m_doc->config()->setEol (KateDocumentConfig::eolMac);
         break;
       }
     }
     else if (ch == '\n')
     {
       m_doc->config()->setEol (KateDocumentConfig::eolUnix);
       break;
     }
  }

  // eol detection
  bool lastCharEOL = false;
  if (file.size () > 0)
  {
    file.at (file.size () - 1);

    int ch = file.getch();

    if ((ch == '\n') || (ch == '\r'))
      lastCharEOL = true;
  }

  file.reset ();

  QTextStream stream (&file);
  QTextCodec *codec = m_doc->config()->codec();
  stream.setEncoding(QTextStream::RawUnicode); // disable Unicode headers
  stream.setCodec(codec); // this line sets the mapper to the correct codec

  // flush current content
  clear ();
  
  // cleanup the blocks  
  for (uint i=0; i < m_blocks.size(); i++)
    delete m_blocks[i];
    
  m_blocks.clear ();

  // do the real work
  bool eof = false;
  KateBufBlock *block = 0;

  while (!m_cacheWriteError && !eof && !stream.atEnd())
  {
    block = new KateBufBlock (this, block, 0);
    eof = block->fillBlock (&stream, lastCharEOL);
        
    m_lines = block->endLine ();
    
    if (block->lines() == 0)
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
  
  if (m_lines == 0)
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
    
    // emit the new line count
    emit linesChanged(m_lines);
  }
  
  emit loadingFinished ();

  return !m_loadingBorked;
}

bool KateBuffer::canEncode ()
{
  QTextCodec *codec = m_doc->config()->codec();

  kdDebug(13020) << "ENC NAME: " << codec->name() << endl;

  // hardcode some unicode encodings which can encode all chars
  if ((QString(codec->name()) == "UTF-8") || (QString(codec->name()) == "ISO-10646-UCS-2"))
    return true;

  for (uint i=0; i < m_lines; i++)
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

  if ( !file.open( IO_WriteOnly ) )
  {
    return false; // Error
  }

  QTextCodec *codec = m_doc->config()->codec();

  // disable Unicode headers
  stream.setEncoding(QTextStream::RawUnicode);

  // this line sets the mapper to the correct codec
  stream.setCodec(codec);

  QString eol = m_doc->config()->eolString ();

  QString tabs;
  if (m_doc->configFlags() & KateDocument::cfReplaceTabs)
    tabs.fill (QChar(' '), m_doc->config()->tabWidth());

  for (uint i=0; i < m_lines; i++)
  {
    TextLine::Ptr textLine = plainLine(i);
  
    if (textLine)
    {    
      // if enabled strip the trailing spaces !
      if (m_doc->configFlags() & KateDocument::cfRemoveSpaces)
      {
        // replace tabs or not ;)
        if (m_doc->configFlags() & KateDocument::cfReplaceTabs)
          stream << textLine->withoutTrailingSpaces().replace (QChar('\t'), tabs);
        else
          stream << textLine->withoutTrailingSpaces();
      }
      else
      {
        // replace tabs or not ;)
        if (m_doc->configFlags() & KateDocument::cfReplaceTabs)
        {
          QString l = textLine->string();
          stream << l.replace (QChar('\t'), tabs);
        }
        else
          stream << textLine->string();
      }
      
      if ((i+1) < m_lines)
        stream << eol;
    }
  }

  file.close ();

  m_loadingBorked = false;
  
  return (file.status() == IO_Ok);
}

/**
 * Return line @p i
 */
TextLine::Ptr KateBuffer::line(uint i)
{
  KateBufBlock *buf = findBlock(i);

  if (!buf)
    return 0;

  if (buf->needHighlight())
  {
    buf->setNeedHighlight (false);

    if (m_highlightedTo > buf->startLine())
    {
      needHighlight (buf, buf->startLine(), buf->endLine());
    }
  }

  if ((m_highlightedRequested <= i) && (m_highlightedTo <= i))
  {
    m_highlightedRequested = buf->endLine();

    pleaseHighlight (m_highlightedTo, buf->endLine());
  }

  return buf->line (i - buf->startLine());
}

bool KateBuffer::needHighlight(KateBufBlock *buf, uint startLine, uint endLine)
{
  // no hl around, no stuff to do
  if (!m_highlight)
    return false;

  // nothing to update, still up to date ;)
  if (!m_hlUpdate)
    return false;
    
  // we tried to start in a line behind this buf block !
  if (startLine >= (buf->startLine()+buf->lines()))
    return false;

  // get the previous line, if we start at the beginning of this block
  // take the last line of the previous block
  TextLine::Ptr prevLine = 0;
  
  if ((startLine == buf->startLine()) && buf->prev())
  {
    KateBufBlock *blk = buf->prev();

    if ((blk->state() > KateBufBlock::stateLoaded) && (blk->lines() > 0))
      prevLine = blk->line (blk->lines() - 1);
    else
      prevLine = blk->lastLine();
  }
  else if ((startLine > buf->startLine()) && (startLine <= buf->endLine()))
  {
    prevLine = buf->line(startLine - buf->startLine() - 1);
  }
  
  if (!prevLine)
    prevLine = new TextLine ();
  
  bool line_continue = prevLine->hlLineContinue();
  
  QMemArray<short> ctxNum, endCtx;
  ctxNum.duplicate (prevLine->ctxArray ()); 

  // does we need to emit a signal for the folding changes ?
  bool codeFoldingUpdate = false;
  
  // here we are atm, start at start line in the block
  uint current_line = startLine - buf->startLine();
  
  // does we need to continue
  bool stillcontinue=false;
   
  // loop over the lines of the block, from startline to endline or end of block
  // if stillcontinue forces us to do so
  while ( (current_line < buf->lines())
          && (stillcontinue || ((current_line + buf->startLine()) <= endLine)) )
  {    
    // current line  
    TextLine::Ptr textLine = buf->line(current_line);
  
    endCtx.duplicate (textLine->ctxArray ());

    QMemArray<signed char> foldingList;
    m_highlight->doHighlight(ctxNum, textLine, line_continue, &foldingList);

    //
    // indentation sensitive folding
    //
    bool indentChanged = false;
    if (m_highlight->foldingIndentationSensitive())
    {
      // get the indentation array of the previous line to start with !
      QMemArray<unsigned short> indentDepth;
      indentDepth.duplicate (prevLine->indentationDepthArray());

      // current indentation of this line      
      uint iDepth = textLine->indentDepth(m_tabWidth);

      // this line is empty, beside spaces, use indentation depth of the previous line !
      if (textLine->firstChar() == -1)
      {
        // do this to get skipped empty lines indent right, which was given in the indenation array
        if (!prevLine->indentationDepthArray().isEmpty())
          iDepth = (prevLine->indentationDepthArray())[prevLine->indentationDepthArray().size()-1];
        else
          iDepth = prevLine->indentDepth(m_tabWidth);
        
        // run over empty lines ;)
        indentChanged = true;
      }
       
      // query the next line indentation, if we are at the end of the block
      // use the first line of the next buf block
      uint nextLineIndentation = 0;
      
      if ((current_line+1) < buf->lines())
      {
        if (buf->line(current_line+1)->firstChar() == -1)
          nextLineIndentation = iDepth;
        else
          nextLineIndentation = buf->line(current_line+1)->indentDepth(m_tabWidth);
      }
      else
      {
        if (buf->next())
        {
          KateBufBlock *blk = buf->next();
    
          if ((blk->state() > KateBufBlock::stateLoaded) && (blk->lines() > 0))
          {
            if (blk->line (0)->firstChar() == -1)
              nextLineIndentation = iDepth;
            else
              nextLineIndentation = blk->line (0)->indentDepth(m_tabWidth);
          }
          else
          {
            if (blk->firstLineOnlySpaces())
              nextLineIndentation = iDepth;
            else
              nextLineIndentation = blk->firstLineIndentation();
          }
        }
      }

      // recalculate the indentation array for this line, query if we have to add
      // a new folding start, this means newIn == true !
      bool newIn = false;
      if ((iDepth > 0) && (indentDepth.isEmpty() || (indentDepth[indentDepth.size()-1] < iDepth)))
      {
        indentDepth.resize (indentDepth.size()+1);
        indentDepth[indentDepth.size()-1] = iDepth;
        newIn = true;
      }
      else
      {
        for (int z=indentDepth.size()-1; z > -1; z--)
        {
          if (indentDepth[z] > iDepth)
            indentDepth.resize (z);
          else if (indentDepth[z] == iDepth)
            break;
          else if (indentDepth[z] < iDepth)
          {
            indentDepth.resize (indentDepth.size()+1);
            indentDepth[indentDepth.size()-1] = iDepth;
            newIn = true;
            break;
          }
        }
      }
      
      // just for debugging always true to start with !
      indentChanged = indentChanged || (indentDepth.size() != textLine->indentationDepthArray().size())
                                    || (indentDepth != textLine->indentationDepthArray());
 
      // set the new array in the textline !
      textLine->setIndentationDepth (indentDepth);
      
      // add folding start to the list !
      if (newIn)
      {
        foldingList.resize (foldingList.size() + 1);
        foldingList[foldingList.size()-1] = 1;
      }
      
      // calculate how much end folding symbols must be added to the list !
      // remIn gives you the count of them
      uint remIn = 0;
      
      for (int z=indentDepth.size()-1; z > -1; z--)
      {
        if (indentDepth[z] > nextLineIndentation)
          remIn++;
        else
          break;
      }

      if (remIn > 0)
      {
        foldingList.resize (foldingList.size() + remIn);

        for (uint z= foldingList.size()-remIn; z < foldingList.size(); z++)
          foldingList[z] = -1;
      }
    }

    bool foldingChanged = (foldingList.size() != textLine->foldingListArray().size())
                          || (foldingList != textLine->foldingListArray());

    if (foldingChanged)
      textLine->setFoldingList(foldingList);

    bool retVal_folding = false;
    m_regionTree.updateLine(current_line + buf->startLine(), &foldingList, &retVal_folding, foldingChanged);

    codeFoldingUpdate = codeFoldingUpdate | retVal_folding;

    line_continue=textLine->hlLineContinue();

    ctxNum.duplicate (textLine->ctxArray());

    if ( indentChanged || (endCtx.size() != ctxNum.size()) )
    {
      stillcontinue = true;
    }
    else
    {
      stillcontinue = false;

      if ((ctxNum != endCtx))
        stillcontinue = true;
    }

    // move around the lines
    prevLine = textLine;
    
    // increment line
    current_line++;
  }

  // tag the changed lines !
  emit tagLines (startLine, current_line + buf->startLine());

  // emit that we have changed the folding
  if (codeFoldingUpdate)
    emit codeFoldingUpdated();

  // if we are at the last line of the block + we still need to continue
  // return the need of that !
  return stillcontinue && ((current_line+1) == buf->lines());
}

void KateBuffer::updateHighlighting(uint from, uint to, bool invalidate)
{
   //kdDebug()<<"KateBuffer::updateHighlight("<<from<<","<<to<<","<<invalidate<<")"<<endl;
   if (!m_hlUpdate)
    return;

   //kdDebug()<<"passed the no update check"<<endl;

   if (from > m_highlightedTo )
     from = m_highlightedTo;

   uint done = 0;
   bool endStateChanged = true;

   while (done < to)
   {
      KateBufBlock *buf = findBlock(from);
      if (!buf)
         return;

      if (buf->needHighlight() || invalidate || m_highlightedTo < buf->endLine())
      {
         uint fromLine = buf->startLine();
         uint tillLine = buf->endLine();

         if (!buf->needHighlight() && invalidate)
         {
            if (to < tillLine)
               tillLine = to;

            if (from > fromLine)
            {
               if (m_highlightedTo > from)
                 fromLine = from;
               else if (m_highlightedTo > fromLine)
                 fromLine = m_highlightedTo;
            }
         }

         buf->setNeedHighlight (false);

   //kdDebug()<<"Calling need highlight: "<<fromLine<<","<<tillLine<<endl;
         endStateChanged = needHighlight (buf, fromLine, tillLine);

         buf->markDirty();
      }

      done = buf->endLine();
      from = done;
   }
   if (invalidate)
   {
      if (endStateChanged)
         m_highlightedTo = done;
      m_highlightedRequested = done;
   }
   else
   {
      if (done > m_highlightedTo)
         m_highlightedTo = done;
   }
}

void KateBuffer::invalidateHighlighting()
{
   m_highlightedTo = 0;
   m_highlightedRequested = 0;
}

void KateBuffer::pleaseHighlight(uint from, uint to)
{
  if (to > m_highlightedEnd)
    m_highlightedEnd = to;

  if (m_highlightedEnd < from)
    return;

  //
  // this calc makes much of the responsiveness
  //
  m_highlightedSteps = ((m_highlightedEnd-from) / 5) + 1;
  if (m_highlightedSteps < 100)
    m_highlightedSteps = 100;
  else if (m_highlightedSteps > 2000)
    m_highlightedSteps = 2000;

  uint till = from + m_highlightedSteps;
  if (till > m_highlightedEnd)
    till = m_highlightedEnd;

  updateHighlighting(from, till, false);

  m_highlightedTill = till;
  if (m_highlightedTill >= m_highlightedEnd)
  {
    m_highlightedTill = 0;
    m_highlightedEnd = 0;
    m_highlightTimer.stop();
  }
  else
  {
    m_highlightTimer.start(100, true);
  }
}

void KateBuffer::pleaseHighlight()
{
  uint till = m_highlightedTill + m_highlightedSteps;

  if (m_highlightedSteps == 0)
    till += 100;

  if (m_highlightedEnd > m_lines)
    m_highlightedEnd = m_lines;

  if (till > m_highlightedEnd)
    till = m_highlightedEnd;

  updateHighlighting(m_highlightedTill, till, false);

  m_highlightedTill = till;
  if (m_highlightedTill >= m_highlightedEnd)
  {
    m_highlightedTill = 0;
    m_highlightedEnd = 0;
    m_highlightedSteps = 0;
    m_highlightTimer.stop();
  }
  else
  {
    m_highlightTimer.start(100, true);
  }
}

/**
 * Return line @p i without triggering highlighting
 */
TextLine::Ptr KateBuffer::plainLine(uint i)
{
   KateBufBlock *buf = findBlock(i);
   if (!buf)
      return 0;

   return buf->line(i - buf->startLine());
}

/**
 * Return line @p i without triggering highlighting
 */
QString KateBuffer::textLine(uint i)
{
  TextLine::Ptr l = plainLine(i);
  
  if (!l)
    return QString();
  
  return l->string();
}

void KateBuffer::insertLine(uint i, TextLine::Ptr line)
{
  //kdDebug()<<"bit debugging"<<endl;
  //kdDebug()<<"bufferblock count: "<<m_blocks.count()<<endl;

  uint index = 0;
  KateBufBlock *buf;
  if (i == m_lines)
    buf = findBlock(i-1, &index);
  else
    buf = findBlock(i, &index);

  if (!buf)
    return;

  buf->insertLine(i -  buf->startLine(), line);

  if (m_highlightedTo > i)
    m_highlightedTo++;
   
  m_lines++;

  // last sync block adjust
  if (m_lastInSyncBlock > index)
    m_lastInSyncBlock = index;

  m_regionTree.lineHasBeenInserted (i);
}

void
KateBuffer::removeLine(uint i)
{
   uint index = 0;
   KateBufBlock *buf = findBlock(i, &index);

   if (!buf)
     return;
   
  buf->removeLine(i -  buf->startLine());

  if (m_highlightedTo > i)
    m_highlightedTo--;

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

  m_regionTree.lineHasBeenRemoved (i);
}

void KateBuffer::changeLine(uint i)
{
  KateBufBlock *buf = findBlock(i);

  if (buf)
    buf->markDirty ();
}

void KateBuffer::setLineVisible(unsigned int lineNr, bool visible)
{
   //kdDebug(13000)<<"void KateBuffer::setLineVisible(unsigned int lineNr, bool visible)"<<endl;
   TextLine::Ptr l=plainLine(lineNr);
   if (l)
   {
     l->setVisible(visible);
     changeLine (lineNr);
   }
   
   //kdDebug(13000)<<QString("Invalid line %1").arg(lineNr)<<endl;
}

uint KateBuffer::length ()
{
  uint l = 0;

  for (uint i = 0; i < count(); i++)
  {
    TextLine::Ptr line = plainLine(i);
    
    if (line)
      l += line->length();
  }

  return l;
}

int KateBuffer::lineLength ( uint i )
{
  TextLine::Ptr l = plainLine(i);
  
  if (!l)
    return -1;
  
  return l->length();
}

QString KateBuffer::text()
{
  QString s;

  for (uint i = 0; i < count(); i++)
  {
    TextLine::Ptr textLine = plainLine(i);
  
    if (textLine)
    {
      s.append (textLine->string());
      
      if ((i+1) < count())
        s.append('\n');
    }
  }

  return s;
}

QString KateBuffer::text ( uint startLine, uint startCol, uint endLine, uint endCol, bool blockwise )
{
  if ( blockwise && (startCol > endCol) )
    return QString ();

  QString s;

  if (startLine == endLine)
  {
    if (startCol > endCol)
      return QString ();

    TextLine::Ptr textLine = plainLine(startLine);

    if ( !textLine )
      return QString ();

    return textLine->string(startCol, endCol-startCol);
  }
  else
  {
    for (uint i = startLine; (i <= endLine) && (i < count()); i++)
    {
      TextLine::Ptr textLine = plainLine(i);

      if ( !blockwise )
      {
        if (i == startLine)
          s.append (textLine->string(startCol, textLine->length()-startCol));
        else if (i == endLine)
          s.append (textLine->string(0, endCol));
        else
          s.append (textLine->string());
      }
      else
      {
        s.append (textLine->string (startCol, endCol - startCol));
      }

      if ( i < endLine )
        s.append('\n');
    }
  }

  return s;
}

void KateBuffer::dumpRegionTree()
{
  m_regionTree.debugDump();
}

// BEGIN KateBufBlockList
KateBufBlockList::KateBufBlockList ()
 : m_count (0),
   m_first (0),
   m_last (0)
{
}

KateBufBlockList::~KateBufBlockList ()
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
    
void KateBufBlockList::remove (KateBufBlock *buf)
{
  if (buf->list)
    buf->list->removeInternal (buf);
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
// END KateBufBlockList

// BEGIN KateBufBlock

KateBufBlock::KateBufBlock(KateBuffer *parent, KateBufBlock *prev, KateBufBlock *next)
: m_state (KateBufBlock::stateLoaded),
  m_firstLineIndentation (0),
  m_firstLineOnlySpaces (true),
  m_lastLine (0),
  m_vmblock (0),
  m_vmblockSize (0),
  b_needHighlight (true),
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
  else
    m_startLine = 0;

  if (m_next)
    m_next->m_prev = this;
    
  // init the block with one empty line, state is loaded, not parsed !
  // which means after init is no string list around, only rawdata
  m_rawData.resize (sizeof(uint) + 1);
  char* buf = m_rawData.data ();
  uint length = 0;
  memcpy(buf, (char *) &length, sizeof(uint));
  char attr = TextLine::flagNoOtherData;
  memcpy(buf+sizeof(uint), (char *) &attr, 1);
  m_lines = 1;
  
  // we are a loaded block
  m_parent->m_loadedBlocks.append (this);
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
    m_parent->vm()->free(m_vmblock);
  
  // remove me from the list I belong
  KateBufBlockList::remove (this);
}

bool KateBufBlock::fillBlock (QTextStream *stream, bool lastCharEOL)
{
  if (m_state != KateBufBlock::stateLoaded)
    return true;

  m_lines = 0;
  m_rawData.resize (KATE_AVG_BLOCK_SIZE);
  
  bool eof = false; 
  char *buf = m_rawData.data ();
  uint pos = 0;
  char attr = TextLine::flagNoOtherData;

  uint size = 0;
  while (size < KATE_AVG_BLOCK_SIZE)
  {
    QString line = stream->readLine();

    if (lastCharEOL || !stream->atEnd() || !line.isNull())
    {
      uint length = line.length ();
      size = pos + sizeof(uint) + (sizeof(QChar)*length) + 1;

      if (size > m_rawData.size ())
      {
        m_rawData.resize (size);
        buf = m_rawData.data ();
      }

      memcpy(buf+pos, (char *) &length, sizeof(uint));
      pos += sizeof(uint);

      if (!line.isNull())
      {
        memcpy(buf+pos, (char *) line.unicode(), sizeof(QChar)*length);
        pos += sizeof(QChar)*length;
      }

      memcpy(buf+pos, (char *) &attr, 1);
      pos += 1;

      m_lines++;
    }

    if (stream->atEnd() && line.isNull())
    {
      eof = true;
      break;
    }
  }

  if (size < m_rawData.size())
  {
    m_rawData.resize (size);
  }

  return eof;
}

TextLine::Ptr KateBufBlock::line(uint i)
{
  // take care that the string list is around !!!
  if (m_state == KateBufBlock::stateSwapped)
  {
    swapIn ();
    buildStringList ();
  }
  else if (m_state == KateBufBlock::stateLoaded)
    buildStringList ();

  return m_stringList[i];
}

void KateBufBlock::insertLine(uint i, TextLine::Ptr line)
{
  // take care that the string list is around !!!
  if (m_state == KateBufBlock::stateSwapped)
  {
    swapIn ();
    buildStringList ();
  }
  else if (m_state == KateBufBlock::stateLoaded)
    buildStringList ();

  m_stringList.insert (m_stringList.begin()+i, line);
  m_lines++;
}

void KateBufBlock::removeLine(uint i)
{
  // take care that the string list is around !!!
  if (m_state == KateBufBlock::stateSwapped)
  {
    swapIn ();
    buildStringList ();
  }
  else if (m_state == KateBufBlock::stateLoaded)
    buildStringList ();

  m_stringList.erase (m_stringList.begin()+i);
  m_lines--;
}

void KateBufBlock::markDirty ()
{
  if (m_state == KateBufBlock::stateClean)
  {    
    // we are dirty, raw + swap data has no longer any use
  
    // cu raw data
    m_rawData.resize (0);
    
    // cu swap data
    if (m_vmblock)
      m_parent->vm()->free(m_vmblock);

    m_vmblock = 0;
    m_vmblockSize = 0;

    setState (KateBufBlock::stateDirty);
  }
}

void KateBufBlock::setState (KateBufBlock::State state)
{
  if (state == m_state)
    return;
   
  // remove me from my old list if any !
  KateBufBlockList::remove (this);
     
  switch (state)
  {    
    case KateBufBlock::stateLoaded:
      if (m_parent->m_loadedBlocks.count() >=  KATE_MAX_BLOCKS_LOADED)
        m_parent->m_loadedBlocks.first()->swapOut ();
      
      m_parent->m_loadedBlocks.append (this);
      break;
    
    case KateBufBlock::stateClean:
      if (m_parent->m_cleanBlocks.count() >=  KATE_MAX_BLOCKS_CLEAN)
        m_parent->m_cleanBlocks.first()->disposeStringList ();
    
      m_parent->m_cleanBlocks.append (this);
      break;
    
    case KateBufBlock::stateDirty:
      if (m_parent->m_dirtyBlocks.count() >=  KATE_MAX_BLOCKS_DIRTY)
        m_parent->m_dirtyBlocks.first()->flushStringList ();
    
      m_parent->m_dirtyBlocks.append (this);
      break;
      
    default:
      break;
  }

  m_state = state;
}

void KateBufBlock::swapIn ()
{
  if (m_state != KateBufBlock::stateSwapped)
    return;
  
  m_rawData.resize(m_vmblockSize);
  
  // what to do if that fails ?
  if (!m_parent->vm()->copyBlock(m_rawData.data(), m_vmblock, 0, m_vmblockSize))
    m_parent->m_cacheReadError = true;
  
  setState (KateBufBlock::stateLoaded);
}

void KateBufBlock::buildStringList()
{
  if (m_state != KateBufBlock::stateLoaded)
    return;

  char *buf = m_rawData.data();
  char *end = buf + m_rawData.count();

  while(buf < end)
  {
    TextLine::Ptr textLine = new TextLine ();
    buf = textLine->restore (buf);
    m_stringList.push_back (textLine);
  }

  //kdDebug(13020)<<"stringList.count = "<< m_stringList.size()<<" should be "<< (m_endState.lineNr - m_beginState.lineNr) <<endl;

  if (m_lines > 0)
  {
    m_lastLine = m_stringList[m_lines - 1];
  }
  else
  {
    m_lastLine = 0;
  }
  
  m_firstLineIndentation = 0;
  m_firstLineOnlySpaces = true;
  
  setState (KateBufBlock::stateClean);
}

void KateBufBlock::swapOut ()
{
  if (m_state != KateBufBlock::stateLoaded)
    return;

  m_vmblock = m_parent->vm()->allocate(m_rawData.count());
  m_vmblockSize = m_rawData.count();
  
  if (!m_rawData.isEmpty())
  {
    if (!m_parent->vm()->copyBlock(m_vmblock, m_rawData.data(), 0, m_rawData.count()))
    {
      if (m_vmblock)
        m_parent->vm()->free(m_vmblock);

      m_vmblock = 0;
      m_vmblockSize = 0;
    
      m_parent->m_cacheWriteError = true;
      
      return;
    }
  }

  // dispose the raw data
  m_rawData.resize (0);

  setState (KateBufBlock::stateSwapped);
  
  return;
}

void KateBufBlock::disposeStringList()
{
  if (m_state != KateBufBlock::stateClean)
    return;
  
  if (m_lines > 0)
  {
    m_firstLineIndentation = m_stringList[0]->indentDepth (m_parent->tabWidth());
    m_firstLineOnlySpaces = (m_stringList[0]->firstChar() == -1);
    m_lastLine = m_stringList[m_lines - 1];
  }
  else
  {
    m_firstLineIndentation = 0;
    m_firstLineOnlySpaces = true;
    m_lastLine = 0;
  }

  m_stringList.clear();
  
  setState (KateBufBlock::stateLoaded);
}

void KateBufBlock::flushStringList()
{
  if (m_state != KateBufBlock::stateDirty)
    return;
     
  // Calculate size.
  uint size = 0;
  for(TextLine::List::const_iterator it = m_stringList.begin(); it != m_stringList.end(); ++it)
    size += (*it)->dumpSize ();

  m_rawData.resize (size);
  char *buf = m_rawData.data();

  // Dump textlines
  for(TextLine::List::iterator it = m_stringList.begin(); it != m_stringList.end(); ++it)
    buf = (*it)->dump (buf);

  setState (KateBufBlock::stateClean);
}

// END KateBufBlock

// kate: space-indent on; indent-width 2; replace-tabs on;
