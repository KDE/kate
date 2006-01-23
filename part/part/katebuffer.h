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

#ifndef __KATE_BUFFER_H__
#define __KATE_BUFFER_H__

#include "katetextline.h"
#include "katecodefoldinghelpers.h"

#include <kvmallocator.h>

#include <QObject>

class KateLineInfo;
class KateDocument;
class KateHighlighting;
class KateBufBlockList;
class KateBuffer;
class KateFileLoader;

class QTextCodec;

/**
 * The KateBufBlock class contains an amount of data representing
 * a certain number of lines.
 *
 * @author Waldo Bastian <bastian@kde.org>
 * @author Christoph Cullmann <cullmann@kde.org>
 */
class KateBufBlock
{
  friend class KateBufBlockList;

  public:
    /**
     * Create an empty block. (empty == ONE line)
     * @param parent buffer the block belongs to
     * @param prev previous bufblock in the list
     * @param next next bufblock in the list
     * @param stream stream to load the content from, if any given
     */
    KateBufBlock ( KateBuffer *parent, KateBufBlock *prev = 0, KateBufBlock *next = 0,
                   KateFileLoader *stream = 0 );

    /**
     * destroy this block and take care of freeing all mem
     */
    ~KateBufBlock ();

  private:
    /**
     * fill the block with the lines from the given stream
     * @param stream stream to load data from
     */
    void fillBlock (KateFileLoader *stream);

  public:
    /**
     * state flags
     */
    enum State
    {
      stateSwapped = 0,
      stateClean = 1,
      stateDirty = 2
    };

    /**
     * returns the current state of this block
     * @return state
     */
    State state () const { return m_state; }

  public:
    /**
     * return line @p i
     * The first line of this block is line 0.
     * if you modifiy this line, please mark the block as dirty
     * @param i line to return
     * @return line pointer
     */
    KateTextLine::Ptr line(int i);

    /**
     * insert @p line in front of line @p i
     * marks the block dirty
     * @param i where to insert
     * @param line line pointer
     */
    void insertLine(int i, KateTextLine::Ptr line);

    /**
     * remove line @p i
     * marks the block dirty
     * @param i line to remove
     */
    void removeLine(int i);

    /**
     * mark this block as dirty, will invalidate the swap data
     * insert/removeLine will mark the block dirty itself
     */
    void markDirty ();

  public:
    /**
     * startLine
     * @return first line in block
     */
    inline int startLine () const { return m_startLine; };

    /**
     * update the first line, needed to keep it up to date
     * @param line new startLine
     */
    inline void setStartLine (int line) { m_startLine = line; }

    /**
     * first line behind this block
     * @return line behind block
     */
    inline int endLine () const { return m_startLine + m_lines; }

    /**
     * lines in this block
     * @return lines
     */
    inline int lines () const { return m_lines; }

    /**
     * prev block
     * @return previous block
     */
    inline KateBufBlock *prev () { return m_prev; }

    /**
     * next block
     * @return next block
     */
    inline KateBufBlock *next () { return m_next; }

  /**
   * methodes to swap in/out
   */
  private:
    /**
     * swap in the kvmallocater data, create string list
     */
    void swapIn ();

    /**
     * swap our string list out, delete it !
     */
    void swapOut ();

  private:
    /**
     * VERY IMPORTANT, state of this block
     * this uchar indicates if the block is swapped, loaded, clean or dirty
     */
    KateBufBlock::State m_state;

    /**
     * IMPORTANT, start line
     */
    int m_startLine;

    /**
     * IMPORTANT, line count
     */
    int m_lines;

    /**
     * here we swap our stuff
     */
    KVMAllocator::Block *m_vmblock;

    /**
     * swapped size
     */
    int m_vmblockSize;

    /**
     * list of textlines
     */
    QVector<KateTextLine::Ptr> m_stringList;

    /**
     * parent buffer.
     */
    KateBuffer* m_parent;

    /**
     * prev block
     */
    KateBufBlock *m_prev;

    /**
     * next block
     */
    KateBufBlock *m_next;

  private:
    /**
     * list pointer, to which list I belong
     * list element pointers for the KateBufBlockList ONLY !!!
     */
    KateBufBlockList *list;

    /**
     * prev list item
     */
    KateBufBlock *listPrev;

    /**
     * next list item
     */
    KateBufBlock *listNext;
};

/**
 * list which allows O(1) inserts/removes
 * will not delete the elements on remove
 * will use the next/prevNode pointers in the KateBufBlocks !
 * internal use: loaded/clean/dirty block lists
 *
 * @author Christoph Cullmann <cullmann@kde.org>
 */
class KateBufBlockList
{
  public:
    /**
     * Default Constructor
     */
    KateBufBlockList ();

  public:
    /**
     * count of blocks in this list
     * @return count of blocks
     */
    inline int count() const { return m_count; }

    /**
     * first block in this list or 0
     * @return head of list
     */
    inline KateBufBlock *first () { return m_first; };

    /**
     * last block in this list or 0
     * @return end of list
     */
    inline KateBufBlock *last () { return m_last; };

    /**
     * is buf the last block?
     * @param buf block to test
     * @return is this block the first one?
     */
    inline bool isFirst (KateBufBlock *buf) { return m_first == buf; };

    /**
     * is buf the last block?
     * @param buf block to test
     * @return is this block the last one?
     */
    inline bool isLast (KateBufBlock *buf) { return m_last == buf; };

    /**
     * append a block to this list !
     * will remove it from the list it belonged before !
     * @param buf block to append
     */
    void append (KateBufBlock *buf);

    /**
     * remove the block from the list it belongs to !
     * @param buf block to remove
     */
    inline static void remove (KateBufBlock *buf)
    {
      if (buf->list)
        buf->list->removeInternal (buf);
    }

  private:
    /**
     * internal helper for remove
     * @param buf block to remove
     */
    void removeInternal (KateBufBlock *buf);

  private:
    /**
     * count of blocks in list
     */
    int m_count;

    /**
     * first block
     */
    KateBufBlock *m_first;

    /**
     * last block
     */
    KateBufBlock *m_last;
};

/**
 * The KateBuffer class maintains a collections of lines.
 * It allows to maintain state information in a lazy way.
 * It handles swapping out of data using secondary storage.
 *
 * It is designed to handle large amounts of text-data efficiently
 * with respect to CPU and memory usage.
 *
 * @author Waldo Bastian <bastian@kde.org>
 * @author Christoph Cullmann <cullmann@kde.org>
 */
class KateBuffer : public QObject
{
  Q_OBJECT

  friend class KateBufBlock;

  public:
    /**
     * maximal loaded block count
     * @return max loaded blocks
     */
    inline static int maxLoadedBlocks () { return m_maxLoadedBlocks; }

    /**
     * modifier for max loaded blocks limit
     * @param count new limit
     */
    static void setMaxLoadedBlocks (int count);

  private:
    /**
     * global max loaded blocks limit
     */
    static int m_maxLoadedBlocks;

  public:
    /**
     * Create an empty buffer.
     * @param doc parent document
     */
    KateBuffer (KateDocument *doc);

    /**
     * Goodbye buffer
     */
    ~KateBuffer ();

  public:
    /**
     * start some editing action
     */
    void editStart ();

    /**
     * finish some editing action
     */
    void editEnd ();

    /**
     * were there changes in the current running
     * editing session?
     * @return changes done?
     */
    inline bool editChanged () const { return editChangesDone; }

    /**
     * dirty lines start
     * @return start line
     */
    inline int editTagStart () const { return editTagLineStart; }

    /**
     * dirty lines end
     * @return end line
     */
    inline int editTagEnd () const { return editTagLineEnd; }

    /**
     * line inserted/removed?
     * @return line inserted/removed?
     */
    inline bool editTagFrom () const { return editTagLineFrom; }

  private:
    /**
     * edit session recursion
     */
    int editSessionNumber;

    /**
     * is a edit session running
     */
    bool editIsRunning;

    /**
     * dirty lines start at line
     */
    int editTagLineStart;

    /**
     * dirty lines end at line
     */
    int editTagLineEnd;

    /**
     * a line was inserted or removed
     */
    bool editTagLineFrom;

    /**
     * changes done?
     */
    bool editChangesDone;

  public:
    /**
     * Clear the buffer.
     */
    void clear();

    /**
     * Open a file, use the given filename
     * @param m_file filename to open
     * @return success
     */
    bool openFile (const QString &m_file);

    /**
     * was the last loading broken because of not enough tmp disk space ?
     * (will be reseted on successful save of the file, user gets warning if he really wants to do it)
     * @return was loading borked?
     */
    bool loadingBorked () const { return m_loadingBorked; }

    /**
     * is this file a binary?
     * @return binary file?
     */
    bool binary () const { return m_binary; }

    /**
     * Can the current codec handle all chars
     * @return chars can be encoded
     */
    bool canEncode ();

    /**
     * Save the buffer to a file, use the given filename + codec + end of line chars (internal use of qtextstream)
     * @param m_file filename to save to
     * @return success
     */
    bool saveFile (const QString &m_file);

  public:
    /**
     * Return line @p i
     */
    inline KateTextLine::Ptr line(int i)
    {
      KateBufBlock *buf = findBlock(i);
      if (!buf)
        return KateTextLine::Ptr();

      if (i < m_lineHighlighted)
        return buf->line (i - buf->startLine());

      return line_internal (buf, i);
    }

  private:
    /**
     * line needs hl
     */
     KateTextLine::Ptr line_internal (KateBufBlock *buf, int i);

     inline void addIndentBasedFoldingInformation(QVector<int> &foldingList,bool addindent,int deindent);
     inline void updatePreviousNotEmptyLine(KateBufBlock *blk,int current_line,bool addindent,int deindent);

  public:
    /**
     * Return line @p i without triggering highlighting
     */
    inline KateTextLine::Ptr plainLine(int i)
    {
      KateBufBlock *buf = findBlock(i);
      if (!buf)
        return KateTextLine::Ptr();

      return buf->line(i - buf->startLine());
    }

    /**
     * Return the total number of lines in the buffer.
     */
    inline int count() const { return m_lines; }

  private:
    /**
     * Find the block containing line @p i
     * index pointer gets filled with index of block in m_blocks
     * index only valid if returned block != 0 !
     */
    KateBufBlock *findBlock (int i, int *index = 0)
    {
      // out of range !
      if (i < 0 || i >= m_lines)
        return 0;

      if ((m_blocks[m_lastFoundBlock]->startLine() <= i) && (m_blocks[m_lastFoundBlock]->endLine() > i))
      {
        if (index)
          (*index) = m_lastFoundBlock;

        return m_blocks[m_lastFoundBlock];
      }

      return findBlock_internal (i, index);
    }

    KateBufBlock *findBlock_internal (int i, int *index = 0);

  public:
    /**
     * Mark line @p i as changed !
     */
    void changeLine(int i);

    /**
     * Insert @p line in front of line @p i
     */
    void insertLine(int i, KateTextLine::Ptr line);

    /**
     * Remove line @p i
     */
    void removeLine(int i);

  public:
    inline int countVisible () { return m_lines - m_regionTree.getHiddenLinesCount(m_lines); }

    inline int lineNumber (int visibleLine) { return m_regionTree.getRealLine (visibleLine); }

    inline int lineVisibleNumber (int line) { return m_regionTree.getVirtualLine (line); }

    inline void lineInfo (KateLineInfo *info, int line) { m_regionTree.getLineInfo(info,line); }

    inline int tabWidth () const { return m_tabWidth; }

  public:
    void setTabWidth (int w);

    /**
     * Use @p highlight for highlighting
     *
     * @p highlight may be 0 in which case highlighting
     * will be disabled.
     */
    void setHighlight (int hlMode);

    KateHighlighting *highlight () { return m_highlight; };

    /**
     * Invalidate highlighting of whole buffer.
     */
    void invalidateHighlighting();

    KateCodeFoldingTree *foldingTree () { return &m_regionTree; };

    void codeFoldingColumnUpdate(int lineNr);

  private:
    /**
     * Highlight information needs to be updated.
     *
     * @param buf The buffer being processed.
     * @param startState highlighting state of last line before range
     * @param from first line in range
     * @param to last line in range
     * @param invalidat should the rehighlighted lines be tagged ?
     *
     * @returns true when the highlighting in the next block needs to be updated,
     * false otherwise.
     */
    bool doHighlight (KateBufBlock *buf, int from, int to, bool invalidate);
    bool isEmptyLine(KateTextLine::Ptr textline);
  Q_SIGNALS:
    /**
     * Emittend if codefolding returned with a changed list
     */
    void codeFoldingUpdated();

    /**
     * Emitted when the highlighting of a certain range has
     * changed.
     */
    void tagLines(int start, int end);

  private:
    /**
     * document we belong too
     */
    KateDocument *m_doc;

    /**
     * current line count
     */
    int m_lines;

    /**
     * ALL blocks
     * in order of linenumbers
     */
    QVector<KateBufBlock*> m_blocks;

    /**
     * last block where the start/end line is in sync with real life
     */
    int m_lastInSyncBlock;

    /**
     * last block found by findBlock, there to make searching faster
     */
    int m_lastFoundBlock;

    /**
     * status of the cache read/write errors
     * write errors get handled, read errors not really atm
     */
    bool m_cacheReadError;
    bool m_cacheWriteError;

    /**
     * had we cache error while loading ?
     */
    bool m_loadingBorked;

    /**
     * binary file loaded ?
     */
    bool m_binary;

  /**
   * highlighting & folding relevant stuff
   */
  private:
    /**
     * current highlighting mode or 0
     */
    KateHighlighting *m_highlight;

    /**
     * folding tree
     */
    KateCodeFoldingTree m_regionTree;

    // for the scrapty indent sensitive langs
    int m_tabWidth;

    int m_lineHighlightedMax;
    int m_lineHighlighted;

  /**
     * number of dynamic contexts causing a full invalidation
     */
    int m_maxDynamicContexts;

  /**
   * only used from the KateBufBlocks !
   */
  private:
    /**
     * all not swapped blocks !
     */
    KateBufBlockList m_loadedBlocks;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
