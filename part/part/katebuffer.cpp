/* This file is part of the KDE libraries
   Copyright (c) 2000 Waldo Bastian <bastian@kde.org>

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

// $Id$

#include "katebuffer.h"
#include "katebuffer.moc"

#include "katedocument.h"
#include "katehighlight.h"

#include <kvmallocator.h>
#include <kdebug.h>

#include <qfile.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qtextcodec.h>

#include <assert.h>

// SOME LIMITS, may need testing what limits are clever
#define AVG_BLOCK_SIZE                         24000
#define LOAD_N_BLOCKS_AT_ONCE       3

/**
  Some private classes
*/                    
class KateBufFileLoader
{
  public:
    KateBufFileLoader (const QString &m_file) : 
      file (m_file), stream (&file)
    { 
    }

  public:
    QFile file;
    QTextStream stream;
    KateBufBlock *prev;
};

/**
 * The KateBufBlock class contains an amount of data representing
 * a certain number of lines.
 */
class KateBufBlock
{
  friend class KateBuffer;

  public:
   /*
    * Create an empty block.
    */
   KateBufBlock (KateBufBlock *prev, KVMAllocator *vm);
   
   ~KateBufBlock ();

   /**
    * Fill block with lines from @p data1 and @p data2.
    * The first line starts at @p data1[@p dataStart].
    * If @p last is true, all bytes from @p data2 are stored.
    * @return The number of bytes stored form @p data2
    */
   bool fillBlock (QTextStream *stream);

   /**
    * Create a valid stringList.
    * Post Condition: b_stringListValid is true.
    */
   void buildStringList();

   /**
    * Copy stringlist back to raw data.
    * Post Condition: b_rawDataValid is true.
    */
   void flushStringList();
   
   /**
    * Dispose of a stringList.
    * Post Condition: b_stringListValid is false.
    */
   void disposeStringList();

   /**
    * Dispose of raw data.
    * Post Condition: b_rawDataValid is false.
    */
   void disposeRawData();

   /**
    * Post Condition: b_vmDataValid is true, b_rawDataValid is false
    */
   void swapOut ();

   /**
    * Swaps raw data from secondary storage.
    * Post Condition: b_rawDataValid is true.
    */
   void swapIn ();

   /**
    * Dispose of swap data.
    * Post Condition: b_vmDataValid is false.
    */
   void disposeSwap ();

   /**
    * Return line @p i
    * The first line of this block is line 0.
    */
   TextLine::Ptr line(uint i);

   /**
    * Insert @p line in front of line @p i
    */
   void insertLine(uint i, TextLine::Ptr line);

   /**
    * Remove line @p i
    */
   void removeLine(uint i);
   
   /**
    * First line in block
    */
   inline uint startLine () { return m_startLine; };
   
   inline void setStartLine (uint line)
   {
     m_startLine = line;
   }
   
   /**
    * First line behind this block
    */
   inline uint endLine () { return m_startLine + m_lines; }
   
   /**
    * Lines in this block
    */
   inline uint lines () { return m_lines; }
   
   inline TextLine::Ptr lastLine () { return m_lastLine; }

  private:
    // IMPORTANT, start line + lines in block
    uint m_startLine;
    uint m_lines;
 
    // Used for context & hlContinue flag if this bufblock has no stringlist
    TextLine::Ptr m_lastLine;

    // here we swap our stuff
    KVMAllocator *m_vm;
    KVMAllocator::Block *m_vmblock;
    uint m_vmblockSize;
    bool b_vmDataValid;

    /**
     * raw data, if m_codec != 0 it is still the dump file content,
     * if m_codec == 0 it is the dumped string list
     */ 
    QByteArray m_rawData;
    bool b_rawDataValid;

    /**
     * list of textlines
     */
    TextLine::List m_stringList;
    bool b_stringListValid;

    // Buffer requires highlighting.
    bool b_needHighlight; 
};
     
/**
 * Create an empty buffer. (with one block with one empty line)
 */
KateBuffer::KateBuffer(KateDocument *doc) : QObject (doc),
  m_lastInSyncBlock (0),
  m_highlight (0),
  m_doc (doc),
  m_loader (0),
  m_vm (0),
  m_regionTree (0),
  m_highlightedTill (0),
  m_highlightedEnd (0)
{
  m_blocks.setAutoDelete(true);
  
  connect( &m_loadTimer, SIGNAL(timeout()), this, SLOT(loadFilePart()));
  connect( &m_highlightTimer, SIGNAL(timeout()), this, SLOT(slotBufferUpdateHighlight()));
  
  connect( this, SIGNAL(pleaseHighlight(uint,uint)),this,SLOT(slotBufferUpdateHighlight(uint,uint)));
    
  clear();
}     

/**
 * Cleanup on destruction
 */
KateBuffer::~KateBuffer()
{ 
  m_blocks.clear ();

  delete m_vm;
  delete m_loader;
}

/**
 * Check if we allready have to much loaded stuff
 */
void KateBuffer::checkLoadedMax ()
{
  if (m_loadedBlocks.count() > 40)
  {
    KateBufBlock *buf2 = m_loadedBlocks.take(2);
    buf2->swapOut ();
  }
}

/**
 * Check if we allready have to much clean stuff
 */
void KateBuffer::checkCleanMax ()
{
  if (m_cleanBlocks.count() > 10)
  {
    checkLoadedMax ();
    
    KateBufBlock *buf2 = m_cleanBlocks.take(2);
    buf2->disposeStringList();
    m_loadedBlocks.append(buf2);
  }
}

/**
 * Check if we allready have to much dirty stuff
 */
void KateBuffer::checkDirtyMax ()
{
  if (m_dirtyBlocks.count() > 10)
  {
    checkLoadedMax ();
    
    KateBufBlock *buf2 = m_dirtyBlocks.take(2);
    buf2->flushStringList(); // Copy stringlist to raw
    buf2->disposeStringList(); // dispose stringlist.
    m_loadedBlocks.append(buf2);
  }
}

/**
 * Load the block out of the swap
 */
void KateBuffer::loadBlock(KateBufBlock *buf)
{
  if (m_loadedBlocks.findRef (buf) > -1)
    return;
 
  // does we have allready to much loaded blocks ?
  checkLoadedMax (); 
  
  // swap the data in
  buf->swapIn ();
 
  m_loadedBlocks.append(buf);
}

/**
 * Here we generate the stringlist out of the raw (or swap) data
 */
void KateBuffer::parseBlock(KateBufBlock *buf)
{
  if (m_cleanBlocks.findRef (buf) > -1)
    return;

  // uh, not even loaded :(
  if (!buf->b_rawDataValid)
    loadBlock(buf);
   
  // does we have allready to much clean blocks ?
  checkCleanMax ();
   
  // now you are clean my little block
  buf->buildStringList();
   
  m_loadedBlocks.removeRef(buf);
  m_cleanBlocks.append(buf);  
}

/**
 * After this call only the string list will be there
 */
void KateBuffer::dirtyBlock(KateBufBlock *buf)
{
  if (m_dirtyBlocks.findRef (buf) > -1)
    return;

  // does we have allready to much dirty blocks ?
  checkDirtyMax (); 
  
  // dispose the dirty raw data
  buf->disposeRawData();
  
  // dispose the swap stuff
  if (buf->b_vmDataValid)
    buf->disposeSwap();
  
  m_cleanBlocks.removeRef(buf);
  m_dirtyBlocks.append(buf);
}

/**
 * Find a block for line i + correct start/endlines for blocks
 */
KateBufBlock *KateBuffer::findBlock(uint i)
{
  if ((i >= m_lines))
     return 0;

  KateBufBlock *buf = 0;
   
  if (m_blocks.current() && (m_lastInSyncBlock >= m_blocks.at()))
  {
    buf = m_blocks.current();
  }
  else
  {
    buf = m_blocks.at (m_lastInSyncBlock);
  }
  
  int lastLine = 0;
  while (buf != 0)
  {
    lastLine = buf->endLine ();
      
    if (i < buf->startLine())
    {
      // Search backwards
      buf = m_blocks.prev ();
    }
    else if (i >= buf->endLine())     
    {     
      // Search forwards
      buf = m_blocks.next();
    }
    else
    {
      // We found the block.
      return buf;     
    }     
      
    if (buf && (m_blocks.at () > m_lastInSyncBlock) && (buf->startLine() != lastLine))
    {     
      buf->setStartLine (lastLine);
      m_lastInSyncBlock = m_blocks.at ();
    }
  }

  return 0;
}
     
void KateBuffer::clear()     
{     
  // reset the folding tree hard !
  delete m_regionTree;
  m_regionTree=new KateCodeFoldingTree(this);
  connect(this,SIGNAL(foldingUpdate(unsigned int , QMemArray<signed char>*,bool*,bool)),m_regionTree,SLOT(updateLine(unsigned int, QMemArray<signed char>*,bool *,bool)));
  connect(m_regionTree,SIGNAL(setLineVisible(unsigned int, bool)), this,SLOT(setLineVisible(unsigned int,bool)));
  
  // delete the last loader
  if (m_loader)
    delete m_loader;
  m_loader = 0;
  
  // cleanup the blocks
  m_cleanBlocks.clear();     
  m_dirtyBlocks.clear();     
  m_loadedBlocks.clear();
  m_blocks.clear();     
  delete m_vm;
  m_vm = new KVMAllocator;
  m_highlight = 0;
  
  // create a bufblock with one line, we need that, only in openFile we won't have that
  KateBufBlock *block = new KateBufBlock(0, m_vm);     
  block->b_rawDataValid = true;
  block->m_rawData.resize (sizeof(uint) + 1);
  char* buf = block->m_rawData.data ();
  uint length = 0;
  memcpy(buf, (char *) &length, sizeof(uint));
  char attr = TextLine::flagNoOtherData;
  memcpy(buf+sizeof(uint), (char *) &attr, 1);   
  block->m_lines++;     
  
  m_blocks.append (block);     
  m_loadedBlocks.append (block);
  
  m_lines = block->m_lines;

  m_highlightedTo = 0;
  m_highlightedRequested = 0;
  m_lastInSyncBlock = 0;
  
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
bool KateBuffer::openFile(const QString &m_file, QTextCodec *codec)
{
  clear();
  
  // here we feed the loader with info
  m_loader = new KateBufFileLoader (m_file);
  
  if ( !m_loader->file.open( IO_ReadOnly ) )
  {
    clear();
    return false; // Error
  }
  
  m_loader->stream.setEncoding(QTextStream::RawUnicode); // disable Unicode headers
  m_loader->stream.setCodec(codec); // this line sets the mapper to the correct codec
  m_loader->prev = 0;  
      
  // trash away the one unneeded allready existing block
  m_loadedBlocks.clear();
  m_blocks.clear();
  m_lines = 0;

  // here the real work will be done
  loadFilePart();
  
  return true;
}

bool KateBuffer::saveFile(const QString &m_file, QTextCodec *codec, const QString &eol)
{ 
  QFile file (m_file);
  QTextStream stream (&file);
  
  if ( !file.open( IO_WriteOnly ) )
  {
    return false; // Error
  }
  
  stream.setEncoding(QTextStream::RawUnicode); // disable Unicode headers
  stream.setCodec(codec); // this line sets the mapper to the correct codec
      
  for (uint i=0; i < m_lines; i++)
  {
    stream << textLine (i);
    
    if (i < (m_lines-1))
      stream << eol;
  }
  
  file.close ();
  
  return (file.status() == IO_Ok);
}

void KateBuffer::loadFilePart()
{
  if (!m_loader)
    return;
  
  bool eof = false;  
    
  for (int i = 0; i < LOAD_N_BLOCKS_AT_ONCE; i++)
  {
    if (m_loader->stream.atEnd())
      eof = true;
    
    if (eof) break;
    
    checkLoadedMax ();
  
    KateBufBlock *block = new KateBufBlock(m_loader->prev, m_vm);
    eof = block->fillBlock (&m_loader->stream);
    
    m_blocks.append (block);
    m_loadedBlocks.append (block);
    
    m_loader->prev = block;
    m_lines = block->endLine ();
  }
  
  if (eof)
  {
     kdDebug(13020)<<"Loading finished.\n";
     
     // trigger the creation of a block with one line if there is no data in the buffer now
     // THIS IS IMPORTANT, OR CRASH WITH EMPTY FILE
     if (m_blocks.isEmpty())
       clear ();
     else
     {
      delete m_loader;
      m_loader = 0;
       emit linesChanged(m_lines);
     }
     
     emit loadingFinished ();
  }
  else if (m_loader)
  {
     emit linesChanged(m_lines);
     m_loadTimer.start(0, true);
  }
}

/**
 * Return line @p i
 */
TextLine::Ptr KateBuffer::line(uint i)
{
  KateBufBlock *buf = findBlock(i);

  if (!buf)
    return 0;

  if (!buf->b_stringListValid)
    parseBlock(buf);

  if (buf->b_needHighlight)
  {
    buf->b_needHighlight = false;
    
    if (m_highlightedTo > buf->startLine())
    {
      needHighlight (buf, buf->startLine(), buf->endLine());
    }
  }

  if ((m_highlightedRequested <= i) && (m_highlightedTo <= i))
  {
    m_highlightedRequested = buf->endLine();
    emit pleaseHighlight (m_highlightedTo, buf->endLine());

    // Check again...
    if (!buf->b_stringListValid)
      parseBlock(buf);
  }

  return buf->line (i - buf->m_startLine);
}

bool KateBuffer::needHighlight(KateBufBlock *buf, uint startLine, uint endLine)
{
  if (!m_highlight)
    return false;

  TextLine::Ptr textLine;
  QMemArray<uint> ctxNum, endCtx;

  uint last_line = buf->lines ();
  uint current_line = startLine - buf->startLine();

  endLine = endLine - buf->startLine(); 

  bool line_continue=false;

  TextLine::Ptr startState = 0;
  
  if (startLine == buf->startLine())
  {
    int pos = m_blocks.findRef (buf);
    if (pos > 0)
    {
      KateBufBlock *blk = m_blocks.at (pos-1);
      
      if ((blk->b_stringListValid) && (blk->lines() > 0))
        startState = blk->line (blk->lines() - 1);
      else
        startState = blk->lastLine();
    }
  } 
  else if ((startLine > buf->startLine()) && (startLine <= buf->endLine()))
    startState = buf->line(startLine - buf->startLine() - 1);
  
  if (startState)
  {
    line_continue=startState->hlLineContinue();
    ctxNum.duplicate (startState->ctxArray ());
  }

  bool stillcontinue=false;
  QMemArray<signed char> foldingList;
  bool retVal_folding;
  bool CodeFoldingUpdated=false;
  do
  {
    textLine = buf->line(current_line);

    if (!textLine)
      break;

    endCtx.duplicate (textLine->ctxArray ());

    foldingList.resize(0);

    m_highlight->doHighlight(ctxNum, textLine, line_continue, &foldingList);
    
    bool foldingChanged= !textLine->isFoldingListValid();
    
    if (!foldingChanged)
      foldingChanged=(foldingList!=textLine->foldingListArray());
    
    if (foldingChanged)
      textLine->setFoldingList(foldingList);
    
    retVal_folding = false;  
    emit foldingUpdate(current_line + buf->startLine(), &foldingList, &retVal_folding, foldingChanged);
    
    CodeFoldingUpdated=CodeFoldingUpdated | retVal_folding;
    
    line_continue=textLine->hlLineContinue();
    
    ctxNum.duplicate (textLine->ctxArray());

    if (endCtx.size() != ctxNum.size())
    {
      stillcontinue = true;
    }
    else
    {
      stillcontinue = false;

      if (ctxNum != endCtx)
        stillcontinue = true;
    }

    current_line++;
  }
  while ((current_line < last_line) && ((current_line < endLine) || stillcontinue));

// FIXME FIXME
#if 0
  if (current_line>=endLine)
  {
    foldingList.resize(0);
    emit foldingUpdate(endLine,&foldingList,&retVal_folding,true);
  }
#endif
  current_line += buf->startLine();
  emit tagLines(startLine, current_line - 1);
  if (CodeFoldingUpdated) emit codeFoldingUpdated();
  return (current_line >= buf->endLine());
}

void KateBuffer::updateHighlighting(uint from, uint to, bool invalidate)
{
//  kdDebug(13020)<< "updateHighlighting: from = " << from << " to = " << to << endl;
   assert(to > 0);
   if (from > m_highlightedTo )
     from = m_highlightedTo;
   uint done = 0;
   bool endStateChanged = true;

   while (done < to)
   {
      KateBufBlock *buf = findBlock(from);
      if (!buf)
         return;

      if (!buf->b_stringListValid)
      {
         parseBlock(buf);
      }

      if (buf->b_needHighlight || invalidate || m_highlightedTo < buf->endLine())
      {
         uint fromLine = buf->startLine();
         uint tillLine = buf->endLine();
         
         if (!buf->b_needHighlight && invalidate)
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
            
         buf->b_needHighlight = false;
         
         endStateChanged = needHighlight (buf, fromLine, tillLine);

          if (buf->b_rawDataValid)
          {
            dirtyBlock(buf);
          }
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

void KateBuffer::slotBufferUpdateHighlight(uint from, uint to)
{
  if (to > m_highlightedEnd)
     m_highlightedEnd = to;
     
  uint till = from + 100;
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

void KateBuffer::slotBufferUpdateHighlight()
{
  uint till = m_highlightedTill + 1000;

  uint max = m_lines;
  if (m_highlightedEnd > max)
    m_highlightedEnd = max;

  if (till > m_highlightedEnd)
     till = m_highlightedEnd;
     
  updateHighlighting(m_highlightedTill, till, false);
  
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

/**
 * Return line @p i without triggering highlighting
 */
TextLine::Ptr KateBuffer::plainLine(uint i)
{
   KateBufBlock *buf = findBlock(i);     
   if (!buf)
      return 0;   

   if (!buf->b_stringListValid)     
   {     
      parseBlock(buf);     
   }
    
   return buf->line(i - buf->m_startLine);
}

/**
 * Return text from line @p i without triggering highlighting
 */
QString KateBuffer::textLine(uint i)
{
   KateBufBlock *buf = findBlock(i);     
   if (!buf)
      return QString();     

   if (!buf->b_stringListValid)     
   {     
      parseBlock(buf);     
   }
   TextLine::Ptr l = buf->line(i - buf->startLine());
   return l->string();
}
     
void KateBuffer::insertLine(uint i, TextLine::Ptr line)
{
  //kdDebug()<<"bit debugging"<<endl;
  //kdDebug()<<"bufferblock count: "<<m_blocks.count()<<endl;

   KateBufBlock *buf;
   if (i == m_lines)
      buf = findBlock(i-1);
   else
      buf = findBlock(i);

  if (!buf)
    return;

   if (!buf->b_stringListValid)
   {
      parseBlock(buf);
   }
   if (buf->b_rawDataValid)
   {
      dirtyBlock(buf);
   }
   
   buf->insertLine(i -  buf->startLine(), line);
   
   if (m_highlightedTo > i)
      m_highlightedTo++;
   m_lines++;
   
   if (m_lastInSyncBlock > m_blocks.findRef (buf))
     m_lastInSyncBlock = m_blocks.findRef (buf);
   
   m_regionTree->lineHasBeenInserted (i);
   updateHighlighting(i, i+2, true);
}

void
KateBuffer::removeLine(uint i)
{
   KateBufBlock *buf = findBlock(i);
   assert(buf);
   if (!buf->b_stringListValid)
   {
      parseBlock(buf);
   }
   if (buf->b_rawDataValid)
   {
      dirtyBlock(buf);
   }
   
  buf->removeLine(i -  buf->startLine());
  
  if (m_highlightedTo > i)
    m_highlightedTo--;

  m_lines--;
  
  // trash away a empty block 
  if (buf->lines() == 0)
  {
    if ((m_lastInSyncBlock > 0) && (m_lastInSyncBlock >= m_blocks.findRef (buf)))
      m_lastInSyncBlock = m_blocks.findRef (buf) -1;
         
    m_cleanBlocks.removeRef(buf);
    m_dirtyBlocks.removeRef(buf);
    m_loadedBlocks.removeRef(buf);
    m_blocks.removeRef(buf);
  }
  else
  {
    if (m_lastInSyncBlock > m_blocks.findRef (buf))
      m_lastInSyncBlock = m_blocks.findRef (buf);
  }

  m_regionTree->lineHasBeenRemoved (i);
}

void KateBuffer::changeLine(uint i)
{
  ////kdDebug(13020)<<"changeLine "<< i<<endl;
  KateBufBlock *buf = findBlock(i);
  assert(buf);
  assert(buf->b_stringListValid);
   
  if (buf->b_rawDataValid)
  {
    dirtyBlock(buf);
  }
}

void KateBuffer::setLineVisible(unsigned int lineNr, bool visible)
{
//   kdDebug(13000)<<"void KateBuffer::setLineVisible(unsigned int lineNr, bool visible)"<<endl;
   TextLine::Ptr l=line(lineNr);
   if (l)
   {
     l->setVisible(visible);
     changeLine (lineNr);
   }
//   else
//   kdDebug(13000)<<QString("Invalid line %1").arg(lineNr)<<endl;
}

uint KateBuffer::length ()
{
  uint l = 0;
  
  for (uint i = 0; i < count(); i++)
  {
    l += line(i)->length();
  }

  return l;
}

int KateBuffer::lineLength ( uint i )
{
  KateBufBlock *buf = findBlock(i);     
  if (!buf)
    return -1;     

  if (!buf->b_stringListValid)     
  {     
    parseBlock(buf);     
  }
  
  TextLine::Ptr l = buf->line(i - buf->startLine());
   
  return l->length();
}

QString KateBuffer::text()
{
  QString s;

  for (uint i = 0; i < count(); i++)
  {
    s.append (line(i)->string());
    if ( (i < (count()-1)) )
      s.append('\n');
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
      
    TextLine::Ptr textLine = line(startLine);
    
    if ( !textLine )
      return QString ();
    
    return textLine->string(startCol, endCol-startCol);
  }
  else
  {  
    for (uint i = startLine; (i <= endLine) && (i < count()); i++)
    {
      TextLine::Ptr textLine = line(i);

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
  m_regionTree->debugDump();
}

//-----------------------------------------------------------------

/**
 * The KateBufBlock class contains an amount of data representing
 * a certain number of lines.
 */

/**
 * Create an empty block.
 */
KateBufBlock::KateBufBlock(KateBufBlock *prev, KVMAllocator *vm)
: m_vm (vm),
  m_vmblock (0),
  m_vmblockSize (0),
  b_vmDataValid (false),
  b_rawDataValid (false),
  b_stringListValid (false),
  b_needHighlight (true)
{
  if (prev)
    m_startLine = prev->endLine ();
  else
    m_startLine = 0;

  m_lines = 0;
}

/**
 * Cleanup ;)
 */
KateBufBlock::~KateBufBlock ()
{
  if (b_vmDataValid)
    disposeSwap ();
}

/**
 * Fill block with unicode data from stream
 */     
bool KateBufBlock::fillBlock (QTextStream *stream)     
{
  bool eof = false;
  uint lines = 0;
  
  m_rawData.resize (AVG_BLOCK_SIZE);
  char *buf = m_rawData.data ();
  uint pos = 0;
  char attr = TextLine::flagNoOtherData;
   
  uint size = 0;
  while (size < AVG_BLOCK_SIZE)
  {    
    QString line = stream->readLine();
     
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
    
    lines++;
    
    if (stream->atEnd() && line.isNull())
    {
      eof = true;
      break;
    }
  }
  
  if (pos < m_rawData.size())
  {
    m_rawData.resize (size);
  }
          
  m_lines = lines;
  b_rawDataValid = true;
   
  return eof;     
}     
     
/**     
 * Swaps raw data to secondary storage.     
 * Uses the filedescriptor @p swap_fd and the file-offset @p swap_offset     
 * to store m_rawSize bytes.     
 */     
void KateBufBlock::swapOut ()     
{
   //kdDebug(13020)<<"KateBufBlock: swapout this ="<< this<<endl;
   assert(b_rawDataValid);
   // TODO: Error checking and reporting (?)
   
   if (!b_vmDataValid)
   {     
      m_vmblock = m_vm->allocate(m_rawData.count());
      m_vmblockSize = m_rawData.count();
      
      if (!m_rawData.isEmpty())     
      {       
         m_vm->copy(m_vmblock, m_rawData.data(), 0, m_rawData.count());     
      }     
      b_vmDataValid = true;     
   }     
   disposeRawData();     
}
     
/**
 * Swaps m_rawSize bytes in from offset m_vmDataOffset in the file     
 * with file-descirptor swap_fd.     
 */     
void KateBufBlock::swapIn ()     
{     
   //kdDebug(13020)<<"KateBufBlock: swapin this ="<< this<<endl;
   assert(b_vmDataValid);
   assert(!b_rawDataValid);
   assert(m_vmblock);
   m_rawData.resize(m_vmblockSize);     
   m_vm->copy(m_rawData.data(), m_vmblock, 0, m_vmblockSize);     
   b_rawDataValid = true;     
}
     
/**     
 * Create a valid stringList.     
 */     
void KateBufBlock::buildStringList()     
{     
  //kdDebug(13020)<<"KateBufBlock: buildStringList this ="<< this<<endl;
  assert(m_stringList.empty());

  char *buf = m_rawData.data();
  char *end = buf + m_rawData.count();

  while(buf < end)
  {
    TextLine::Ptr textLine = new TextLine();
    buf = textLine->restore (buf);
    m_stringList.push_back (textLine);
  }

  //kdDebug(13020)<<"stringList.count = "<< m_stringList.size()<<" should be "<< (m_endState.lineNr - m_beginState.lineNr) <<endl;

  if (m_lines > 0)
    m_lastLine = m_stringList[m_lines - 1];
  
  assert(m_stringList.size() == m_lines);
  b_stringListValid = true;
  //kdDebug(13020)<<"END: KateBufBlock: buildStringList LINES: "<<m_endState.lineNr - m_beginState.lineNr<<endl;
}

/**
 * Flush string list
 * Copies a string list back to the raw buffer.
 */
void KateBufBlock::flushStringList()
{
  //kdDebug(13020)<<"KateBufBlock: flushStringList this ="<< this<<endl;
  assert(b_stringListValid);
  assert(!b_rawDataValid);

  if (m_lines > 0)
    m_lastLine = m_stringList[m_lines - 1];
  
  // Calculate size.
  uint size = 0;
  for(TextLine::List::const_iterator it = m_stringList.begin(); it != m_stringList.end(); ++it)
    size += (*it)->dumpSize ();

  m_rawData.resize (size);
  char *buf = m_rawData.data();
   
  // Dump textlines
  for(TextLine::List::iterator it = m_stringList.begin(); it != m_stringList.end(); ++it)
    buf = (*it)->dump (buf);
   
  assert(buf-m_rawData.data() == (int)size);
  b_rawDataValid = true;
}

/**
 * Dispose of a stringList.
 */
void KateBufBlock::disposeStringList()
{
   //kdDebug(13020)<<"KateBufBlock: disposeStringList this = "<< this<<endl;
   assert(b_rawDataValid || b_vmDataValid);
   
   if (m_lines > 0)
    m_lastLine = m_stringList[m_lines - 1];
   
   m_stringList.clear();
   b_stringListValid = false;
}

/**
 * Dispose of raw data.
 */
void KateBufBlock::disposeRawData()
{
   //kdDebug(13020)<< "KateBufBlock: disposeRawData this = "<< this<<endl;
   assert(b_stringListValid || b_vmDataValid);
   b_rawDataValid = false;
   m_rawData.resize (0);
}

/**
 * Dispose of data in vm
 */     
void KateBufBlock::disposeSwap()     
{
  m_vm->free(m_vmblock);
  m_vmblock = 0;
  m_vmblockSize = 0;     
  b_vmDataValid = false;     
}     

/**
 * Return line @p i
 * The first line of this block is line 0.
 */
TextLine::Ptr KateBufBlock::line(uint i)
{     
   assert(b_stringListValid);     
   assert(i < m_stringList.size());
 
   return m_stringList[i];
}

void KateBufBlock::insertLine(uint i, TextLine::Ptr line)
{
   assert(b_stringListValid);
   assert(i <= m_stringList.size());
   
   m_stringList.insert (m_stringList.begin()+i, line);
   m_lines++;
}

void KateBufBlock::removeLine(uint i)
{
   assert(b_stringListValid);
   assert(i < m_stringList.size());

   m_stringList.erase (m_stringList.begin()+i);
   m_lines--;
}
