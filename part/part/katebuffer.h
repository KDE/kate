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

#ifndef _KATE_BUFFER_H_
#define _KATE_BUFFER_H_

#include "katetextline.h"

#include <qptrlist.h>
#include <qobject.h>
#include <qtimer.h>

class KateCodeFoldingTree;
class KateLineInfo;
class KateBufBlock;
class KateBufFileLoader;

class KVMAllocator;

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
    KateBuffer(class KateDocument *doc);

    /**
     * Goodbye buffer
     */
    ~KateBuffer();

    KateDocument* document() const { return m_doc; }

  public slots:
    void setOpenAsync (bool async)
    {
      m_openAsync = async;
    }

    bool openAsync () const
    {
      return m_openAsync;
    }

    /**
     * Open a file, use the given filename + codec (internal use of qtextstream)
     */
    bool openFile (const QString &m_file);

    /**
     * Can the current codec handle all chars
     */
    bool canEncode ();

    /**
     * Save the buffer to a file, use the given filename + codec + end of line chars (internal use of qtextstream)
     */
    bool saveFile (const QString &m_file);

    /**
     * Return the total number of lines in the buffer.
     */
    inline uint count() const
    {
      return m_lines;
    }

    uint countVisible ();

    uint lineNumber (uint visibleLine);

    uint lineVisibleNumber (uint line);

    void lineInfo (KateLineInfo *info, unsigned int line);

    KateCodeFoldingTree *foldingTree ();

    inline void setHlUpdate (bool b)
    {
      m_hlUpdate = b;
    }

    void dumpRegionTree ();

    /**
     * Return line @p i
     */
    TextLine::Ptr line(uint i);

    /**
     * Return line @p i without triggering highlighting
     */
    TextLine::Ptr plainLine(uint i);

    /**
     * Return text from line @p i without triggering highlighting
     */
    QString textLine(uint i, bool withoutTrailingSpaces = false);

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

    /**
     * Use @p highlight for highlighting
     *
     * @p highlight may be 0 in which case highlighting
     * will be disabled.
     */
    void setHighlight(class Highlight *highlight);

    class Highlight *highlight () { return m_highlight; };

    /**
     * Update the highlighting.
     *
     * PRE-condition:
     *   All lines prior to @p from have been highlighted already.
     *
     * POST-condition:
     *   All lines till at least @p to haven been highlighted.
     */
    void updateHighlighting(uint from, uint to, bool invalidate);

    /**
     * Invalidate highlighting of whole buffer.
     */
    void invalidateHighlighting();

    /**
     * Get the whole text in the buffer as a string.
     */
    QString text();

    /**
     * Get the text between the two given positions.
     */
    QString text(uint startLine, uint startCol, uint endLine, uint endCol, bool blockwise = false);

    uint length ();

    int lineLength ( uint line );

    /**
     * change the visibility of a given line
     */
    void setLineVisible (unsigned int lineNr, bool visible);

  signals:
    /**
     * Emitted during loading when the line count changes.
     */
    void linesChanged(int lines);

    /**
     * Emitted when some code folding related attributes changed
     */
    void foldingUpdate(unsigned int , QMemArray<signed char>* ,bool *changed,bool foldingChanged);

    /**
     * Emittend if codefolding returned with a changed list
     */
    void codeFoldingUpdated();

    /**
     * Emitted when the highlighting of a certain range has
     * changed.
     */
    void tagLines(int start, int end);

    /**
     * Advice to update highlighting a certain range.
     */
    void pleaseHighlight(uint from, uint to);

    /**
     * Loading of the file finished
     */
    void loadingFinished ();

    /**
     * Emitted each time text is inserted into a pre-existing line, including appends.
     * Does not include newly inserted lines at the moment. ?need
     */
    void textInserted(TextLine::Ptr line, uint pos, uint len);

    /**
     * Emitted whenever a line is inserted before @p line, becoming itself line @ line.
     */
    void lineInsertedBefore(TextLine::Ptr linePtr, uint lineNum);

    /**
     * Emitted each time text is removed from a line, including truncates and space removal.
     */
    void textRemoved(TextLine::Ptr line, uint pos, uint len);

    /**
     * Emitted when a line is deleted.
     */
    void lineRemoved(uint line);

    /**
     * Emmitted when text from @p line was wrapped at position pos onto line @p nextLine.
     */
    void textWrapped(TextLine::Ptr line, TextLine::Ptr nextLine, uint pos);

    /**
     * Emitted each time text from @p nextLine was upwrapped onto @p line.
     */
    void textUnWrapped(TextLine::Ptr line, TextLine::Ptr nextLine, uint pos, uint len);

  private:
    friend class TextLine;

    enum changeTypes {
      TextInserted,
      TextRemoved,
      TextWrapped,
      TextUnWrapped
    };

    /**
     * Internal method only used by TextLine, to notify of changes to the line.
     */
    void emitTextChanged(uint type, TextLine::Ptr thisLine, uint pos, uint len, TextLine::Ptr* nextLine = 0L);

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

    void checkLoadedMax ();
    void checkCleanMax ();
    void checkDirtyMax ();

    /**
     * Highlight information needs to be updated.
     *
     * @param buf The buffer being processed.
     * @param startState highlighting state of last line before range
     * @param from first line in range
     * @param to last line in range
     *
     * @returns true when the highlighting in the next block needs to be updated,
     * false otherwise.
     */
    bool needHighlight(KateBufBlock *buf, uint from, uint to);

  private slots:
    /**
     * Load a part of the file that is currently loading.
     */
    void loadFilePart();

    void slotBufferUpdateHighlight (uint,uint);
    void slotBufferUpdateHighlight ();

  private:
    bool m_openAsync;

    bool m_hlUpdate;

    uint m_lines;
    uint m_highlightedTo; // The highest line with correct highlight info
    uint m_highlightedRequested; // The highest line that we requested highlight for

    uint m_lastInSyncBlock;  // last block where the start/end line is in sync with real life

    class Highlight *m_highlight;
    class KateDocument *m_doc;

    // stuff we need to load a file
    KateBufFileLoader *m_loader;
    QTimer m_loadTimer;

    // ALL blocks
    QPtrList<KateBufBlock> m_blocks;

    // List of blocks that can be swapped out.
    QPtrList<KateBufBlock> m_loadedBlocks;

    // List of blocks that can be disposed.
    QPtrList<KateBufBlock> m_cleanBlocks;

    // List of blocks that are dirty.
    QPtrList<KateBufBlock> m_dirtyBlocks;

    class KVMAllocator *m_vm;

    // folding tree
    class KateCodeFoldingTree *m_regionTree;

    QTimer m_highlightTimer;

    uint m_highlightedTill;
    uint m_highlightedEnd;
};

#endif
