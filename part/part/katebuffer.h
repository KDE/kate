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

#ifndef _KATE_BUFFER_H_
#define _KATE_BUFFER_H_

#include "kateglobal.h"
#include "katetextline.h"

#include <kvmallocator.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>
#include <qobject.h>
#include <qtimer.h>

class KateBufBlock;
class KateBufFileLoader;
class KateBufState;
class QTextCodec;

/**
 * The KateBuffer class maintains a collections of lines.
 * It allows to maintain state information in a lazy way.
 * It handles swapping out of data using secondary storage.
 *
 * It is designed to handle large amounts of text-data efficiently
 * with respect to CPU and memory usage.
 *
 * @author Waldo Bastian <bastian@kde.org>
 */
class KateBuffer : public QObject
{
   Q_OBJECT
public:
   /**
    * Create an empty buffer.
    */
   KateBuffer();
   ~KateBuffer();

   /**
    * Insert a file at line @p line in the buffer.
    * Using @p codec to decode the file.
    */
   void insertFile(uint line, const QString &file, QTextCodec *codec);

   /**
    * Insert a block of data at line @p line in the buffer.
    * Using @p codec to decode the file.
    */
   void insertData(uint line, const QByteArray &data, QTextCodec *codec);

   /**
    * Return the total number of lines in the buffer.
    */
   uint count();

   /**
    * Return line @p i
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
    * Change line @p i
    */
   void changeLine(uint i);

   /**
    * Clear the buffer.
    */
   void clear();

signals:
   /**
    * Emitted during loading.
    */
   void linesChanged(int lines);
   void needHighlight(uint,uint);

protected:
   /**
    * Make sure @p buf gets loaded.
    */
   void loadBlock(KateBufBlock *buf);

   /**
    * Make sure @p buf gets parsed.
    */
   void parseBlock(KateBufBlock *buf);

   /**
    * Mark @p buf dirty.
    */
   void dirtyBlock(KateBufBlock *buf);

   /**
    * Find the block containing line @p i
    */
   KateBufBlock *findBlock(uint i);

   /**
    * Load a part of the file that is currently loading.
    */
   void loadFilePart();

protected slots:
   void slotLoadFile();

protected:
   uint m_totalLines;

   QPtrList<KateBufBlock> m_blocks;
   QPtrList<KateBufFileLoader> m_loader;

   QTimer m_loadTimer;

   // List of parsed blocks that can be disposed.
   QPtrList<KateBufBlock> m_parsedBlocksClean;
   // List of parsed blocks that are dirty.
   QPtrList<KateBufBlock> m_parsedBlocksDirty;
   // List of blocks that can be swapped out.
   QPtrList<KateBufBlock> m_loadedBlocks;

   KVMAllocator *m_vm;
};

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
   uint lineNr;
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

protected:
   /**
    * Make line @p i the current line
    */
   void seek(uint i);

   /**
    * Create a valid stringList from intern format.
    */
   void buildStringListFast();

protected:
   TextLine::List m_stringList;
   TextLine::List::iterator m_stringListIt;
   int m_stringListCurrent;

   QByteArray m_rawData1;
   int m_rawData1Start;
   QByteArray m_rawData2;
   int m_rawData2End;
   uint m_rawSize;
   bool b_stringListValid;
   bool b_rawDataValid;
   bool b_vmDataValid;
   bool b_appendEOL; // Buffer is not terminated with '\n'.
   bool b_emptyBlock; // Buffer is empty
   uint m_lastLine; // Start of last line if buffer is without EOL.
   KateBufState m_beginState;
   KateBufState m_endState;
   QTextCodec *m_codec;
   KVMAllocator::Block *m_vmblock;
};

#endif
