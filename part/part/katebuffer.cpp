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
#include "katedocument.h"
#include "katehighlight.h"

// Includes for reading file
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <qfile.h>
#include <qtimer.h>
#include <qtextcodec.h>
#include <qstringlist.h>

#include <assert.h>
#include <kdebug.h>
#define LOADED_BLOCKS_MAX	40
#define DIRTY_BLOCKS_MAX        10

// Somewhat smaller than 8192 so that it will still fit in 8192 after we add 
// some overhead.
#define AVG_BLOCK_SIZE		8000

/**
  Some private classes
*/
                    
class KateBufFileLoader
{
public:
  int fd;
  QByteArray lastBlock;
  int dataStart;
  int blockNr;
  QTextCodec *codec;
};

class KateBufState
{
public:
   KateBufState() { line = new TextLine(); }
   KateBufState(const KateBufState &c)
   { 
     lineNr = c.lineNr;
     line = new TextLine();
     *line = *c.line;
   }
   KateBufState &operator=(const KateBufState &c)
   {
     lineNr = c.lineNr;
     line = new TextLine();
     *line = *c.line;
     return *this;
   }
     
   uint lineNr;
   TextLine::Ptr line; // Used for context & hlContinue flag.
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
   KateBufBlock(const KateBufState &beginState);

   /**
    * Fill block with lines from @p data1 and @p data2.
    * The first line starts at @p data1[@p dataStart].
    * If @p last is true, all bytes from @p data2 are stored.
    * @return The number of bytes stored form @p data2
    */
   int blockFill(int dataStart, QByteArray data1, QByteArray data2, bool last);

   /**
    * Remove the last line from the block. The lastLine is returned
    * at offset @p lastLine in @p data1
    * Pre Condition: b_rawDataValid is true.
    */
   void truncateEOL( int &lastLine, QByteArray &data1 );

   /**
    * Create a valid stringList.
    * Post Condition: b_stringListValid is true.
    */
   void buildStringList();

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

   /**
    * Dispose of raw data.
    * Post Condition: b_rawDataValid is false.
    */
   void disposeRawData();

   /**
    * Post Condition: b_vmDataValid is true, b_rawDataValid is false
    */
   void swapOut(KVMAllocator *vm);

   /**
    * Swaps raw data from secondary storage.
    * Post Condition: b_rawDataValid is true.
    */
   void swapIn(KVMAllocator *vm);

   /**
    * Dispose of swap data.
    * Post Condition: b_vmDataValid is false.
    */
   void disposeSwap(KVMAllocator *vm);

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

private:
   /**
    * Create a valid stringList from intern format.
    */
   void buildStringListFast();

private:
   KateBufState m_beginState;
   KateBufState m_endState;
   TextLine::List m_stringList;
   QByteArray m_rawData1;
   int m_rawData1Start;
   QByteArray m_rawData2;
   int m_rawData2End;
   uint m_rawSize;
   bool b_stringListValid : 1;
   bool b_rawDataValid : 1;
   bool b_vmDataValid : 1;
   bool b_appendEOL : 1; // Buffer is not terminated with '\n'.
   bool b_emptyBlock : 1; // Buffer is empty
   bool b_needHighlight : 1; // Buffer requires highlighting.
   uint m_lastLine; // Start of last line if buffer is without EOL.
   
   QTextCodec *m_codec;
   KVMAllocator::Block *m_vmblock;
};
     
/**     
 * Create an empty buffer.
 */     
KateBuffer::KateBuffer(KateDocument *doc) : QObject (doc),
  m_noHlUpdate (false),
  m_highlight (0),
  m_doc (doc),
  m_vm (0)  
{
  m_blocks.setAutoDelete(true);     
  m_loader.setAutoDelete(true);     
  connect( &m_loadTimer, SIGNAL(timeout()), this, SLOT(slotLoadFile()));     
  clear();     
}     

KateBuffer::~KateBuffer()
{
   delete m_vm;
}
     
/**     
 * Reads @p size bytes from file-descriptor @p fd.     
 */     
static QByteArray readBlock(int fd, int size)     
{     
   QByteArray result(size);     
   int bytesToRead = size;     
   int bytesRead = 0;     
   while(bytesToRead)     
   {     
      int n = ::read(fd, result.data()+bytesRead, bytesToRead);     
      if (n == 0) break; // Done     
      if ((n == -1) && (errno == EAGAIN))
         continue;     
      if (n == -1)     
      {
         // TODO: Do some error handling.     
         break;     
      }     
      bytesRead += n;     
      bytesToRead -= n;     
   }
   //kdDebug(13020)<<"Read = "<< bytesRead<<endl;
   result.truncate(bytesRead);
   return result;
}

/**
 * Writes the bytes buf[begin] to buf[end] to @p fd.
 */
/*
static void writeBlock(int fd, const QByteArray &buf, int begin, int end)
{
   while(begin != end)
   {
      int n = ::write(fd, buf.data()+begin, end - begin);
      if ((n == -1) && (errno == EAGAIN))
         continue;
      if (n == -1)
         return; // TODO: Do some error handling.
      begin += n;     
   }     
}
*/
     
void     
KateBuffer::clear()     
{     
   m_parsedBlocksClean.clear();     
   m_parsedBlocksDirty.clear();     
   m_loadedBlocks.clear();     
   m_loader.clear();
   m_blocks.clear();     
   delete m_vm;     
   m_vm = new KVMAllocator;
   m_highlight = 0;
   KateBufState state;     
   // Initial state.
   state.lineNr = 0;     
   KateBufBlock *block = new KateBufBlock(state);     
   m_blocks.insert(0, block);     
   block->b_rawDataValid = true;     
   block->b_appendEOL = true;     
   block->b_emptyBlock = true;     
   block->m_endState.lineNr++;     
   m_loadedBlocks.append(block);
   m_totalLines = block->m_endState.lineNr;
   m_highlightedTo = 0;
   m_highlightedRequested = 0;
}     

void
KateBuffer::setHighlight(Highlight *highlight)
{
   m_highlight = highlight;
   invalidateHighlighting();
}

/**     
 * Insert a file at line @p line in the buffer.     
 */     
void     
KateBuffer::insertFile(uint line, const QString &file, QTextCodec *codec)
{
  assert(line == 0); // Inserting at other places not yet handled.

  int fd = open(QFile::encodeName(file), O_RDONLY);
  if (fd < 0)
  {
      //kdDebug(13020)<<"Error loading file.\n";
     return; // Do some error propagation here.
  }

  KateBufFileLoader *loader = new KateBufFileLoader;
  loader->fd = fd;
  loader->dataStart = 0;
  loader->blockNr = 0;
  loader->codec = codec;
  m_loader.append(loader);

  loadFilePart();
}

void
KateBuffer::loadFilePart()
{
  const int blockSize = AVG_BLOCK_SIZE;
  const int blockRead = 3; // Read 5 blocks in a row

  KateBufFileLoader *loader = m_loader.first();

  KateBufState state;
  if (loader->blockNr > 0)
  {
     KateBufBlock *block = m_blocks.at(loader->blockNr-1);
     state = block->m_endState;
  }
  else
  {
     // Initial state.
     state.lineNr = 0;
  }
  int startLine = state.lineNr;
  bool eof = false;


  for(int i = 0; i < blockRead; i++)
  {
     QByteArray currentBlock = readBlock(loader->fd, blockSize);
     eof = ((int) currentBlock.size()) != blockSize;
     KateBufBlock *block = new KateBufBlock(state);
     m_blocks.insert(loader->blockNr++, block);
     m_loadedBlocks.append(block);
     if (m_loadedBlocks.count() > LOADED_BLOCKS_MAX)
     {
        KateBufBlock *buf2 = m_loadedBlocks.take(2);
        //kdDebug(13020)<<"swapOut "<<buf2<<endl;
        buf2->swapOut(m_vm);
        assert(m_parsedBlocksClean.find(buf2) == -1);
        assert(m_parsedBlocksDirty.find(buf2) == -1);
     }
     block->m_codec = loader->codec;
     loader->dataStart = block->blockFill(loader->dataStart,
                                  loader->lastBlock, currentBlock, eof);
     state = block->m_endState;     
     //kdDebug(13020)<<"current->ref ="<<currentBlock.nrefs()<<" last->ref ="<<loader->lastBlock.nrefs()<<endl;
     loader->lastBlock = currentBlock;
     if (eof) break;
  }
  
  // make sure lines are calculated - thx waldo ;)
  m_totalLines += state.lineNr - startLine;

  if (eof)
  {
     //kdDebug(13020)<<"Loading finished.\n";
     close( loader->fd );
     m_loader.removeRef(loader);
     
     emit loadingFinished ();
  }
  if (m_loader.count())
  {
      //kdDebug(13020)<<"Starting timer...\n";
     m_loadTimer.start(0, true);
 //JW 0
  }
}


void
KateBuffer::insertData(uint line, const QByteArray &data, QTextCodec *codec)
{
   assert(line == m_totalLines);
   KateBufBlock *prev_block;

   // Remove all preceding empty blocks.
   while(true)
   {
      prev_block = m_blocks.last();
      if (!prev_block || !prev_block->b_emptyBlock)
         break;

      m_totalLines -= prev_block->m_endState.lineNr - prev_block->m_beginState.lineNr;
//kdDebug(13020)<< "Removing empty block "<< prev_block << endl;
      m_blocks.removeRef(prev_block);
      m_parsedBlocksClean.removeRef(prev_block);
      m_parsedBlocksDirty.removeRef(prev_block);
      m_loadedBlocks.removeRef(prev_block);
   }

   KateBufState state;
   if (prev_block)
   {
       state = prev_block->m_endState;
   }
   else
   {
        // Initial state.
       state.lineNr = 0;
   }

  uint startLine = state.lineNr;
  KateBufBlock *block = new KateBufBlock(state);
  m_blocks.append(block);
  m_loadedBlocks.append(block);
  block->m_codec = codec;

  // TODO: We always create a new block.
  // It would be more correct to collect the data in larger blocks.
  // We should do that without unnecasserily copying the data though.
  QByteArray lastData;
  int lastLine = 0;
  if (prev_block && prev_block->b_appendEOL && (prev_block->m_codec == codec))
  {
     // Take the last line of the previous block and add it to the
     // the new block.
     prev_block->truncateEOL(lastLine, lastData);
     m_totalLines--;
  }
  block->blockFill(lastLine, lastData, data, true);
  state = block->m_endState;
  int added = state.lineNr - startLine; 
  m_totalLines += added;
  if (m_highlightedTo > startLine)
     m_highlightedTo += added;
}

void
KateBuffer::slotLoadFile()
{
  loadFilePart();
  emit linesChanged(m_totalLines);
}

KateBufBlock *
KateBuffer::findBlock(uint i)
{
   if ((i >= m_totalLines))
      return 0;

   uint lastLine = 0;
   // This needs a bit of optimisation/caching so that we don't walk
   // through the list every time.
   KateBufBlock *buf;
   for(buf = m_blocks.current(); buf; )
   {
      lastLine = buf->m_endState.lineNr;
      if (i < buf->m_beginState.lineNr)
      {
         // Search backwards
         buf = m_blocks.prev();
      }
      else if ((i >= buf->m_beginState.lineNr) && (i < lastLine))
      {
         // We found the block.
         break;     
      }
      else     
      {     
         // Search forwards
         KateBufBlock *lastBuf = buf;
         buf = m_blocks.next();     
         if (!buf) break; // Should never happen, see asserts below
         
         // Adjust line numbering....     
         if (buf->m_beginState.lineNr != lastLine)     
         {
            int offset = lastLine - buf->m_beginState.lineNr;     
            buf->m_beginState.lineNr += offset;     
            buf->m_endState.lineNr += offset;     
         }
         // Adjust state
         *buf->m_beginState.line = *lastBuf->m_endState.line;
      }
   }

   if (!buf)
   {
      // Huh? Strange, m_totalLines must have been out of sync?
      assert(lastLine == m_totalLines);
      assert(false);
      return 0;
   }
   return buf;
}

/**
 * Return line @p i
 */
TextLine::Ptr
KateBuffer::line(uint i)
{
   KateBufBlock *buf = findBlock(i);
   if (!buf)

      return 0;

   if (!buf->b_stringListValid)
   {
      parseBlock(buf);
   }
   
   if (!m_noHlUpdate)
   {
   if (buf->b_needHighlight)
   {
      buf->b_needHighlight = false;
      if (m_highlightedTo > buf->m_beginState.lineNr)
      {
         needHighlight(buf, buf->m_beginState.line, buf->m_beginState.lineNr, buf->m_endState.lineNr);
         *buf->m_endState.line = *(buf->line(buf->m_endState.lineNr - buf->m_beginState.lineNr - 1));
      }
   }
   if ((m_highlightedRequested <= i) &&
       (m_highlightedTo <= i))
   {
      m_highlightedRequested = buf->m_endState.lineNr;
      emit pleaseHighlight(m_highlightedTo, buf->m_endState.lineNr);

      // Check again...
      if (!buf->b_stringListValid)
      {
         parseBlock(buf);
      }
   }
   }

   return buf->line(i - buf->m_beginState.lineNr);
}

bool
KateBuffer::needHighlight(KateBufBlock *buf, TextLine::Ptr startState, uint startLine,uint endLine) {
  if (!m_highlight)
     return false;

//  kdDebug(13000)<<"needHighlight:startLine:"<< startLine <<" endline:"<<endLine<<endl;

  TextLine::Ptr textLine;
  QMemArray<signed char> ctxNum, endCtx;

  uint last_line = buf->m_endState.lineNr - buf->m_beginState.lineNr;
  uint current_line = startLine - buf->m_beginState.lineNr;

  endLine = endLine - buf->m_beginState.lineNr; 

  bool line_continue=false;

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
//    kdDebug(13000)<<"calling doHighlight for line:"<<current_line<<endl;
    m_highlight->doHighlight(ctxNum, textLine, line_continue,&foldingList);
    retVal_folding=false;
//    kdDebug(13000)<<QString("updateing folding for line %1").arg(current_line+buf->m_beginState.lineNr)<<endl;

    bool foldingChanged= !textLine->isFoldingListValid();
    if (!foldingChanged) foldingChanged=(foldingList!=textLine->foldingListArray());
    if (foldingChanged) textLine->setFoldingList(foldingList);
    emit foldingUpdate(current_line + buf->m_beginState.lineNr,&foldingList,&retVal_folding,foldingChanged);
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

#ifdef __GNUC__
#warning FIXME FIXME
#endif
#if 0
  if (current_line>=endLine)
  {
    foldingList.resize(0);
    emit foldingUpdate(endLine,&foldingList,&retVal_folding,true);
  }
#endif
  current_line += buf->m_beginState.lineNr;
  emit tagLines(startLine, current_line - 1);
  if (CodeFoldingUpdated) emit codeFoldingUpdated();
  return (current_line >= buf->m_endState.lineNr);
}



void
KateBuffer::updateHighlighting(uint from, uint to, bool invalidate)
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

      if (buf->b_needHighlight || invalidate ||
          m_highlightedTo < buf->m_endState.lineNr)
      {
         uint fromLine = buf->m_beginState.lineNr;
         uint tillLine = buf->m_endState.lineNr;
         if (!buf->b_needHighlight && invalidate)
         {
            if (to < tillLine)
               tillLine = to;
            if ((from > fromLine) && (m_highlightedTo > from))
               fromLine = from;
         }
         TextLine::Ptr startState;
         if (fromLine == buf->m_beginState.lineNr)
            startState = buf->m_beginState.line;
         else
            startState = buf->line(fromLine - buf->m_beginState.lineNr - 1);
         buf->b_needHighlight = false;
         endStateChanged = needHighlight(buf, startState, fromLine, tillLine);
         *buf->m_endState.line = *(buf->line(buf->m_endState.lineNr - buf->m_beginState.lineNr - 1));

          if (buf->b_rawDataValid)
          {
            dirtyBlock(buf);
          }
      }

      done = buf->m_endState.lineNr;
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

void
KateBuffer::invalidateHighlighting()
{
   m_highlightedTo = 0;
   m_highlightedRequested = 0;
}

/**
 * Return line @p i as plain QString
 */
QString
KateBuffer::textLine(uint i)
{
   KateBufBlock *buf = findBlock(i);     
   if (!buf)
      return QString();     

   if (!buf->b_stringListValid)     
   {     
      parseBlock(buf);     
   }
   TextLine::Ptr l = buf->line(i - buf->m_beginState.lineNr);
   return l->string();
}
     
void     
KateBuffer::insertLine(uint i, TextLine::Ptr line)
{
   KateBufBlock *buf;
   if (i == m_totalLines)
      buf = findBlock(i-1);
   else
      buf = findBlock(i);

   if (!buf)
   {
      KateBufState state;
      // Initial state.
      state.lineNr = 0;
      buf = new KateBufBlock(state);
      m_blocks.insert(0, buf);
      buf->b_rawDataValid = true;
      m_loadedBlocks.append(buf);
   }

   if (!buf->b_stringListValid)
   {
      parseBlock(buf);
   }
   if (buf->b_rawDataValid)
   {
      dirtyBlock(buf);
   }
   buf->insertLine(i -  buf->m_beginState.lineNr, line);
   if (m_highlightedTo > i)
      m_highlightedTo++;
   m_totalLines++;
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
   buf->removeLine(i -  buf->m_beginState.lineNr);
   if (m_highlightedTo > i)
      m_highlightedTo--;
   m_totalLines--;
   if (buf->m_beginState.lineNr == buf->m_endState.lineNr)
   {
//kdDebug(13020)<< "Removing empty block "<< buf << endl;
      if (buf->b_vmDataValid)
      {
//kdDebug(13020)<< "Empty block still has VM."<< buf << endl;
         assert(false);
         buf->disposeSwap(m_vm);
      }
      m_blocks.removeRef(buf);
      m_parsedBlocksClean.removeRef(buf);
      m_parsedBlocksDirty.removeRef(buf);
      m_loadedBlocks.removeRef(buf);
   }
}

void
KateBuffer::changeLine(uint i)
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

void
KateBuffer::parseBlock(KateBufBlock *buf)
{
   //kdDebug(13020)<<"parseBlock "<< buf<<endl;
   if (!buf->b_rawDataValid)
      loadBlock(buf);
   if (m_parsedBlocksClean.count() > 5)
   {
      KateBufBlock *buf2 = m_parsedBlocksClean.take(2);
      buf2->disposeStringList();
      m_loadedBlocks.append(buf2);
      assert(m_parsedBlocksDirty.find(buf2) == -1);
   }
   buf->buildStringList();
   assert(m_parsedBlocksClean.find(buf) == -1);
   m_parsedBlocksClean.append(buf);
   assert(m_loadedBlocks.find(buf) != -1);
   m_loadedBlocks.removeRef(buf);
}

void
KateBuffer::loadBlock(KateBufBlock *buf)
{
    //kdDebug(13020)<<"loadBlock "<<buf<<endl;
   if (m_loadedBlocks.count() > LOADED_BLOCKS_MAX)
   {
      KateBufBlock *buf2 = m_loadedBlocks.take(2);
      //kdDebug(13020)<<"swapOut "<<buf2<<endl;
      buf2->swapOut(m_vm);
      assert(m_parsedBlocksClean.find(buf2) == -1);
      assert(m_parsedBlocksDirty.find(buf2) == -1);
   }

   //kdDebug(13020)<<"swapIn "<<buf<<endl;
   buf->swapIn(m_vm);
   m_loadedBlocks.append(buf);
   assert(m_parsedBlocksClean.find(buf) == -1);
   assert(m_parsedBlocksDirty.find(buf) == -1);
}

void
KateBuffer::dirtyBlock(KateBufBlock *buf)
{
  // kdDebug(13020)<<"dirtyBlock "<<buf<<endl;
   buf->b_emptyBlock = false;
   if (m_parsedBlocksDirty.count() > DIRTY_BLOCKS_MAX)
   {
      KateBufBlock *buf2 = m_parsedBlocksDirty.take(0);
      buf2->flushStringList(); // Copy stringlist to raw
      buf2->disposeStringList(); // dispose stringlist.
      m_loadedBlocks.append(buf2);
      assert(m_parsedBlocksClean.find(buf2) == -1);
   }
   assert(m_loadedBlocks.find(buf) == -1);
   m_parsedBlocksClean.removeRef(buf);
   m_parsedBlocksDirty.append(buf);
   buf->disposeRawData();
   if (buf->b_vmDataValid)
      buf->disposeSwap(m_vm);
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
  uint l;
  
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
  
  TextLine::Ptr l = buf->line(i - buf->m_beginState.lineNr);
   
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


//-----------------------------------------------------------------

/**
 * The KateBufBlock class contains an amount of data representing
 * a certain number of lines.
 */

/*
 * Create an empty block.
 */
KateBufBlock::KateBufBlock(const KateBufState &beginState)
: m_beginState (beginState),
  m_endState (beginState),
  m_rawData1Start (0),
  m_rawData2End (0),
  m_rawSize (0),
  b_stringListValid (false),
  b_rawDataValid (false),
  b_vmDataValid (false),
  b_appendEOL (false),
  b_emptyBlock (false),
  b_needHighlight (true),
  m_lastLine (0),
  m_codec (0),
  m_vmblock (0)
{
}

/**
 * Remove the last line of this block.
 */
void
KateBufBlock::truncateEOL( int &lastLine, QByteArray &data1 )
{
   assert(b_appendEOL);
   assert(b_rawDataValid);

   data1 = m_rawData2;
   lastLine = m_lastLine;
   b_appendEOL = false;
   m_rawData2End = m_lastLine;
   m_rawSize = m_rawData1.count() - m_rawData1Start + m_rawData2End;

   m_endState.lineNr--;
   if (b_stringListValid)
      m_stringList.pop_back();
}

/**
 * Fill block with lines from @p data1 and @p data2.     
 * The first line starts at @p data1[@p dataStart].     
 * If @p last is true, all bytes from @p data2 are stored.     
 * @return The number of bytes stored form @p data2     
 */     
int     
KateBufBlock::blockFill(int dataStart, QByteArray data1, QByteArray data2, bool last)     
{     
   m_rawData1 = data1;     
   m_rawData1Start = dataStart;     
   m_rawData2 = data2;     

   int lineNr = m_beginState.lineNr;     
     
   const char *p;     
   const char *e;     
   QString lastLine;     
   if (!m_rawData1.isEmpty())     
   {     
      p = m_rawData1.data() + m_rawData1Start;     
      e = m_rawData1.data() + m_rawData1.count();
      while(p < e)     
      {     
         if (*p == '\n')     
         {     
            lineNr++;     
         }     
         p++;     
      }     
   }     
     
   p = m_rawData2.data();     
   e = m_rawData2.data() + m_rawData2.count();     
   const char *l = 0;     
   while(p < e)     
   {     
      if ((*p == '\n') || (*p=='\r'))
      {     
         lineNr++;
	 if ((*p=='\r') &&((p+1)<e)) if (*(p+1)=='\n') p++;     
         l = p+1;     
      }     
      p++;
   }     
     
   // If this is the end of the data OR     
   // if the data did not contain any linebreaks up to now     
   // create a line break at the end of the block.     
   if ((last && (e != l)) ||     
       (l == 0))     
   {     
      if (!m_rawData1.isEmpty() || !m_rawData2.isEmpty())     
      {     
         b_appendEOL = true;     
         if (l)     
            m_lastLine = l - m_rawData2.data();     
         else
            m_lastLine = 0;     
         lineNr++;     
      }     
      l = e;     
   }     
     
   m_rawData2End = l - m_rawData2.data();     
   m_endState.lineNr = lineNr;     
   //kdDebug(13020)<<"blockFill "<<this<<" beginState ="<<m_beginState.lineNr<<"%ld endState ="<< m_endState.lineNr<<endl;
   m_rawSize = m_rawData1.count() - m_rawData1Start + m_rawData2End;
   b_rawDataValid = true;
   return m_rawData2End;     
}     
     
/**     
 * Swaps raw data to secondary storage.     
 * Uses the filedescriptor @p swap_fd and the file-offset @p swap_offset     
 * to store m_rawSize bytes.     
 */     
void     
KateBufBlock::swapOut(KVMAllocator *vm)     
{     
   // kdDebug(13020)<<"KateBufBlock: swapout this ="<< this<<endl;
   assert(b_rawDataValid);
   // TODO: Error checking and reporting (?)
   if (!b_vmDataValid)
   {     
      m_vmblock = vm->allocate(m_rawSize);     
      off_t ofs = 0;
      if (!m_rawData1.isEmpty())     
      {     
         ofs = m_rawData1.count() - m_rawData1Start;     
         vm->copy(m_vmblock, m_rawData1.data()+m_rawData1Start, 0, ofs);     
      }     
      if (!m_rawData2.isEmpty())     
      {     
         vm->copy(m_vmblock, m_rawData2.data(), ofs, m_rawData2End);     
      }     
      b_vmDataValid = true;     
   }     
   disposeRawData();     
}     
     
/**
 * Swaps m_rawSize bytes in from offset m_vmDataOffset in the file     
 * with file-descirptor swap_fd.     
 */     
void     
KateBufBlock::swapIn(KVMAllocator *vm)     
{     
   // kdDebug(13020)<<"KateBufBlock: swapin this ="<< this<<endl;
   assert(b_vmDataValid);
   assert(!b_rawDataValid);
   assert(m_vmblock);     
   m_rawData2.resize(m_rawSize);     
   vm->copy(m_rawData2.data(), m_vmblock, 0, m_rawSize);     
   m_rawData2End = m_rawSize;     
   b_rawDataValid = true;     
}     
     
     
/**     
 * Create a valid stringList.     
 */     
void     
KateBufBlock::buildStringList()     
{     
   //kdDebug(13020)<<"KateBufBlock: buildStringList this ="<< this<<endl;
   assert(m_stringList.empty());
   if (!m_codec && !m_rawData2.isEmpty())
   {     
      buildStringListFast();
      return;     
   }     
     
   const char *p;     
   const char *e;     
   const char *l = 0; // Pointer to start of last line.     
   QString lastLine;     
   if (!m_rawData1.isEmpty())     
   {     
      p = m_rawData1.data() + m_rawData1Start;     
      e = m_rawData1.data() + m_rawData1.count();     
      l = p;     
      while(p < e)     
      {     
         if ((*p == '\n') || (*p == '\r'))
         {
            // TODO: Use codec
            QString line = m_codec->toUnicode(l, (p-l-1)+1);     
            TextLine::Ptr textLine = new TextLine();     
            textLine->append(line.unicode(), line.length());     
            m_stringList.push_back(textLine);     
	    if ((*p=='\r') && ((p+1)<e))  if ((*p+1)=='\n') {p++;}
            l = p+1;
         }     
         p++;     
      }
      if (l < e)     
         lastLine = m_codec->toUnicode(l, (e-l-1)+1);     
   }     
     
   if (!m_rawData2.isEmpty())     
   {     
      p = m_rawData2.data();     
      e = m_rawData2.data() + m_rawData2End;     
      l = p;     
      while(p < e)     
      {     
         if ((*p == '\n') || (*p == '\r'))
         {     
            QString line = m_codec->toUnicode(l, (p-l-1)+1);     
            if (!lastLine.isEmpty())     
            {     
               line = lastLine + line;
               lastLine.truncate(0);     
            }     
            TextLine::Ptr textLine = new TextLine();     
            textLine->append(line.unicode(), line.length());
            m_stringList.push_back(textLine);
	    if ((*p=='\r') && ((p+1)<e)) if (*(p+1)=='\n') {p++;}
            l = p+1;
         }
         p++;
      }

      // If this is the end of the data OR
      // if the data did not contain any linebreaks up to now
      // create a line break at the end of the block.
      if (b_appendEOL)
      {
          //kdDebug(13020)<<"KateBufBlock: buildStringList this ="<<this<<" l ="<< l<<" e ="<<e <<" (e-l)+1 ="<<(e-l)<<endl;
         QString line = m_codec->toUnicode(l, (e-l));
         //kdDebug(13020)<<"KateBufBlock: line ="<< line.latin1()<<endl;
         if (!lastLine.isEmpty())
         {
            line = lastLine + line;
            lastLine.truncate(0);
         }
         TextLine::Ptr textLine = new TextLine();
         textLine->append(line.unicode(), line.length());
         m_stringList.push_back (textLine);
      }
   }
   else
   {
      if (b_appendEOL)
      {
         TextLine::Ptr textLine = new TextLine();
         m_stringList.push_back (textLine);
      }
   }
   assert(m_stringList.size() == (m_endState.lineNr - m_beginState.lineNr));
   b_stringListValid = true;
   b_needHighlight = true;
}

/**
 * Flush string list
 * Copies a string list back to the raw buffer.
 */
void
KateBufBlock::flushStringList()
{
  // kdDebug(13020)<<"KateBufBlock: flushStringList this ="<< this<<endl;
  assert(b_stringListValid);
  assert(!b_rawDataValid);

  // Calculate size.
  uint size = 0;
  for(TextLine::List::const_iterator it = m_stringList.begin(); it != m_stringList.end(); ++it)
    size += (*it)->dumpSize ();
   
  //kdDebug(13020)<<"Size = "<< size<<endl;
  m_rawData2 = QByteArray(size);
  m_rawData2End = size;
  m_rawSize = size;
  char *buf = m_rawData2.data();
   
  // Dump textlines
  for(TextLine::List::iterator it = m_stringList.begin(); it != m_stringList.end(); ++it)
    buf = (*it)->dump (buf);
   
  assert(buf-m_rawData2.data() == (int)size);
  m_codec = 0; // No codec
  b_rawDataValid = true;
}

/**
 * Create a valid stringList from raw data in our own format.
 */
void
KateBufBlock::buildStringListFast()
{
  // kdDebug(13020)<<"KateBufBlock: buildStringListFast this = "<< this<<endl;
  char *buf = m_rawData2.data();
  char *end = buf + m_rawSize;

  while(buf < end)
  {
    TextLine::Ptr textLine = new TextLine();
    buf = textLine->restore (buf);
    m_stringList.push_back (textLine);
  }

  //kdDebug(13020)<<"stringList.count = "<< m_stringList.size()<<" should be "<< (m_endState.lineNr - m_beginState.lineNr) <<endl;
  assert(m_stringList.size() == (m_endState.lineNr - m_beginState.lineNr));
  b_stringListValid = true;
  //b_needHighlight = true;
}

/**
 * Dispose of a stringList.
 */
void
KateBufBlock::disposeStringList()
{
//   kdDebug(13020)<<"KateBufBlock: disposeStringList this = "<< this<<endl;
   assert(b_rawDataValid || b_vmDataValid);
   m_stringList.clear();
   b_stringListValid = false;
   //b_needHighlight = true;
}

/**
 * Dispose of raw data.
 */
void
KateBufBlock::disposeRawData()
{
//   kdDebug(13020)<< "KateBufBlock: disposeRawData this = "<< this<<endl;
   assert(b_stringListValid || b_vmDataValid);
   b_rawDataValid = false;
   m_rawData1 = QByteArray();
   m_rawData1Start = 0;
   m_rawData2 = QByteArray();
   m_rawData2End = 0;
}

/**
 * Dispose of data in vm
 */     
void     
KateBufBlock::disposeSwap(KVMAllocator *vm)     
{
   //kdDebug(13020)<<"KateBufBlock: disposeSwap this = "<< this<<endl;
   assert(b_stringListValid || b_rawDataValid);
   vm->free(m_vmblock);
   m_vmblock = 0;     
   b_vmDataValid = false;     
}     

/**
 * Return line @p i
 * The first line of this block is line 0.
 */
TextLine::Ptr     
KateBufBlock::line(uint i)
{     
   assert(b_stringListValid);     
   assert(i < m_stringList.size());
 
   return m_stringList[i];
}

void
KateBufBlock::insertLine(uint i, TextLine::Ptr line)
{
   assert(b_stringListValid);
   assert(i <= m_stringList.size());
   
   m_stringList.insert(m_stringList.begin()+i, line);
   m_endState.lineNr++;
}

void
KateBufBlock::removeLine(uint i)
{
   assert(b_stringListValid);
   assert(i < m_stringList.size());
   
   m_stringList.erase(m_stringList.begin()+i);
   m_endState.lineNr--;
}

#include "katebuffer.moc"


