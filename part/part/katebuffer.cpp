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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <qfile.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qtextcodec.h>
#include <qstringlist.h>

#include <assert.h>
#include <kdebug.h>

// SOME LIMITS, may need testing what limits are clever
#define LOADED_BLOCKS_MAX                40
#define PARSED_CLEAN_BLOCKS_MAX   10
#define PARSED_DIRTY_BLOCKS_MAX    10
#define AVG_BLOCK_SIZE                         24000
#define LOAD_N_BLOCKS_AT_ONCE       3

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
    KateBufState state;
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
   KateBufState m_beginState;
   KateBufState m_endState;
   
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
   
   bool b_needHighlight; // Buffer requires highlighting.

   KVMAllocator::Block *m_vmblock;
   uint m_vmblockSize;
   bool b_vmDataValid;
};
     
/**     
 * Create an empty buffer.
 */     
KateBuffer::KateBuffer(KateDocument *doc) : QObject (doc),
  m_noHlUpdate (false),
  m_highlight (0),
  m_doc (doc),
  m_loader (0),
  m_vm (0),
  m_regionTree (0)
{
  m_blocks.setAutoDelete(true);     
  
  connect( &m_loadTimer, SIGNAL(timeout()), this, SLOT(loadFilePart()));
    
  clear();
}     

KateBuffer::~KateBuffer()
{ 
  delete m_vm;
  delete m_loader;
}
     
void     
KateBuffer::clear()     
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
  m_parsedBlocksClean.clear();     
  m_parsedBlocksDirty.clear();     
  m_loadedBlocks.clear();
  m_blocks.clear();     
  delete m_vm;     
  m_vm = new KVMAllocator;
  m_highlight = 0;
  
  // create a bufblock with one line, we need that, only in openFile we won't have that
  KateBufState state;
  state.lineNr = 0;      
  KateBufBlock *block = new KateBufBlock(state);     
  m_blocks.insert(0, block);     
  block->b_rawDataValid = true;
  
  block->m_rawData.resize (sizeof(uint) + 1);
  char* buf = block->m_rawData.data ();
  uint length = 0;
  memcpy(buf, (char *) &length, sizeof(uint));
  char attr = TextLine::flagNoOtherData;
  memcpy(buf+sizeof(uint), (char *) &attr, 1);
       
  block->m_endState.lineNr++;     
  m_loadedBlocks.append(block);
  m_totalLines = block->m_endState.lineNr;
  m_highlightedTo = 0;
  m_highlightedRequested = 0;
  
  emit linesChanged(m_totalLines);
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
  m_loader->state.lineNr = 0;  
      
  // trash away the one unneeded allready existing block
  m_loadedBlocks.clear();
  m_blocks.clear();
  m_totalLines = 0;

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
      
  for (uint i=0; i < m_totalLines; i++)
  {
    stream << textLine (i);
    
    if (i < (m_totalLines-1))
      stream << eol;
  }
  
  file.close ();
  
  return (file.status() == IO_Ok);
}

void
KateBuffer::loadFilePart()
{
  if (!m_loader)
    return;
  
  bool eof = false;  
    
  for (int i = 0; i < LOAD_N_BLOCKS_AT_ONCE; i++)
  {
    if (m_loader->stream.atEnd())
      eof = true;
    
    if (eof) break;
  
    KateBufBlock *block = new KateBufBlock(m_loader->state);
    m_blocks.append (block);
    m_loadedBlocks.append (block);
    
    if (m_loadedBlocks.count() > LOADED_BLOCKS_MAX)
    {
      KateBufBlock *buf2 = m_loadedBlocks.take(2);
      buf2->swapOut(m_vm);
      assert(m_parsedBlocksClean.find(buf2) == -1);
      assert(m_parsedBlocksDirty.find(buf2) == -1);
    }
    
    eof = block->fillBlock (&m_loader->stream);
    
    m_loader->state = block->m_endState;
  }
  
  // make sure lines are calculated - thx waldo ;)
  m_totalLines = m_loader->state.lineNr;

  if (eof)
  {
     kdDebug(13020)<<"Loading finished.\n";
     delete m_loader;
     m_loader = 0;
     
     // trigger the creation of a block with one line if there is no data in the buffer now
     // THIS IS IMPORTANT, OR CRASH WITH EMPTY FILE
     if (m_blocks.isEmpty())
       clear ();
     else
       emit linesChanged(m_totalLines);

     emit loadingFinished ();
  }
  else if (m_loader)
  {
     kdDebug(13020)<<"Starting timer...\n";
     emit linesChanged(m_totalLines);
     m_loadTimer.start(0, true);
  }
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
  QMemArray<uint> ctxNum, endCtx;

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

// FIXME FIXME
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
 * Return line @p i without triggering highlighting
 */
TextLine::Ptr
KateBuffer::plainLine(uint i)
{
   KateBufBlock *buf = findBlock(i);     
   if (!buf)
      return 0;   

   if (!buf->b_stringListValid)     
   {     
      parseBlock(buf);     
   }
    
   return buf->line(i - buf->m_beginState.lineNr);
}

/**
 * Return text from line @p i without triggering highlighting
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
  //kdDebug()<<"bit debugging"<<endl;
  //kdDebug()<<"bufferblock count: "<<m_blocks.count()<<endl;

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
   
  m_regionTree->lineHasBeenRemoved (i);
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
   kdDebug(13020)<<"parseBlock "<< buf<<endl;
   
   if (!buf->b_rawDataValid)
      loadBlock(buf);
      
   if (m_parsedBlocksClean.count() > PARSED_CLEAN_BLOCKS_MAX)
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
    kdDebug(13020)<<"loadBlock "<<buf<<endl;
   if (m_loadedBlocks.count() > LOADED_BLOCKS_MAX)
   {
      KateBufBlock *buf2 = m_loadedBlocks.take(2);
      kdDebug(13020)<<"swapOut "<<buf2<<endl;
      buf2->swapOut(m_vm);
      assert(m_parsedBlocksClean.find(buf2) == -1);
      assert(m_parsedBlocksDirty.find(buf2) == -1);
   }

   kdDebug(13020)<<"swapIn "<<buf<<endl;
   buf->swapIn(m_vm);
   m_loadedBlocks.append(buf);
   assert(m_parsedBlocksClean.find(buf) == -1);
   assert(m_parsedBlocksDirty.find(buf) == -1);
}

void
KateBuffer::dirtyBlock(KateBufBlock *buf)
{
   kdDebug(13020)<<"dirtyBlock "<<buf<<endl;
   
   if (m_parsedBlocksDirty.count() > PARSED_DIRTY_BLOCKS_MAX)
   {
      KateBufBlock *buf2 = m_parsedBlocksDirty.take(2);
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

void KateBuffer::dumpRegionTree()
{
  m_regionTree->debugDump();
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
  b_rawDataValid (false),
  b_stringListValid (false),
  b_needHighlight (true),
  m_vmblock (0),
  m_vmblockSize (0),
  b_vmDataValid (false)
{
}

/**
 * Fill block with lines from @p data1 and @p data2.     
 * The first line starts at @p data1[@p dataStart].     
 * If @p last is true, all bytes from @p data2 are stored.     
 * @return The number of bytes stored form @p data2     
 */     
bool KateBufBlock::fillBlock (QTextStream *stream)     
{
  bool eof = false;

  int lineNr = m_beginState.lineNr;     
  
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
    
    lineNr++;
    
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
          
  m_endState.lineNr = lineNr;     
  
  b_rawDataValid = true;
   
  return eof;     
}     
     
/**     
 * Swaps raw data to secondary storage.     
 * Uses the filedescriptor @p swap_fd and the file-offset @p swap_offset     
 * to store m_rawSize bytes.     
 */     
void     
KateBufBlock::swapOut(KVMAllocator *vm)     
{     
   //kdDebug(13020)<<"KateBufBlock: swapout this ="<< this<<endl;
   assert(b_rawDataValid);
   // TODO: Error checking and reporting (?)
   
   if (!b_vmDataValid)
   {     
      m_vmblock = vm->allocate(m_rawData.count());
      m_vmblockSize = m_rawData.count();
      
      if (!m_rawData.isEmpty())     
      {       
         vm->copy(m_vmblock, m_rawData.data(), 0, m_rawData.count());     
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
   //kdDebug(13020)<<"KateBufBlock: swapin this ="<< this<<endl;
   assert(b_vmDataValid);
   assert(!b_rawDataValid);
   assert(m_vmblock);
   m_rawData.resize(m_vmblockSize);     
   vm->copy(m_rawData.data(), m_vmblock, 0, m_vmblockSize);     
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

  char *buf = m_rawData.data();
  char *end = buf + m_rawData.count();

  while(buf < end)
  {
    TextLine::Ptr textLine = new TextLine();
    buf = textLine->restore (buf);
    m_stringList.push_back (textLine);
  }

  //kdDebug(13020)<<"stringList.count = "<< m_stringList.size()<<" should be "<< (m_endState.lineNr - m_beginState.lineNr) <<endl;

  assert(m_stringList.size() == (m_endState.lineNr - m_beginState.lineNr));
  b_stringListValid = true;
  //kdDebug(13020)<<"END: KateBufBlock: buildStringList LINES: "<<m_endState.lineNr - m_beginState.lineNr<<endl;
}

/**
 * Flush string list
 * Copies a string list back to the raw buffer.
 */
void
KateBufBlock::flushStringList()
{
  //kdDebug(13020)<<"KateBufBlock: flushStringList this ="<< this<<endl;
  assert(b_stringListValid);
  assert(!b_rawDataValid);

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
void
KateBufBlock::disposeStringList()
{
   //kdDebug(13020)<<"KateBufBlock: disposeStringList this = "<< this<<endl;
   assert(b_rawDataValid || b_vmDataValid);
   m_stringList.clear();
   b_stringListValid = false;
}

/**
 * Dispose of raw data.
 */
void
KateBufBlock::disposeRawData()
{
   //kdDebug(13020)<< "KateBufBlock: disposeRawData this = "<< this<<endl;
   assert(b_stringListValid || b_vmDataValid);
   b_rawDataValid = false;
   m_rawData.resize (0);
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
   m_vmblockSize = 0;     
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
