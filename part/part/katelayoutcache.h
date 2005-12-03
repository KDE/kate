/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KATELAYOUTCACHE_H
#define KATELAYOUTCACHE_H

#include <ktexteditor/range.h>
#include "katelinerange.h"

class KateRenderer;

namespace KTextEditor { class Document; }

/**
 * This class handles Kate's caching of layouting information (in KateLineLayout
 * and KateTextLayout).  This information is used primarily by both the view and
 * the renderer.
 *
 * We outsource the hardcore layouting logic to the renderer, but other than
 * that, this class handles all manipulation of the layout objects.
 *
 * This is separate from the renderer 1) for clarity 2) so you can have separate
 * caches for separate views of the same document, even for view and printer
 * (if the renderer is made to support rendering onto different targets).
 *
 * @author Hamish Rodda \<rodda@kde.org\>
 */
class KateLayoutCache
{
  public:
    KateLayoutCache(KateRenderer* renderer);

    void clear();

    int viewWidth() const;
    void setViewWidth(int width);

    bool wrap() const;
    void setWrap(bool wrap);

    // BEGIN generic methods to get/set layouts
    /**
     * Returns the KateLineLayout for the specified line.
     *
     * If one does not exist, it will be created and laid out.
     * Layouts which are not directly part of the view will be kept until the
     * cache is full or until they are invalidated by other means (eg. the text
     * changes).
     *
     * \param realLine real line number of the layout to retrieve.
     * \param virtualLine virtual line number. only needed if you think it may have changed
     *                    (ie. basically internal to KateLayoutCache)
     */
    KateLineLayoutPtr line(int realLine, int virtualLine = -1) const;
    /// \overload
    KateLineLayoutPtr line(const KTextEditor::Cursor& realCursor) const;

    /// Returns the layout describing the text line which is occupied by \p realCursor.
    KateTextLayout textLayout(const KTextEditor::Cursor& realCursor) const;

    /// Returns the layout of the specified realLine + viewLine.
    /// if viewLine is -1, return the last.
    KateTextLayout textLayout(uint realLine, int viewLine) const;
    // END

    // BEGIN methods to do with the caching of lines visible within a view
    /// Returns the layout of the corresponding line in the view
    KateTextLayout& viewLine(int viewLine) const;

    // find the view line of the cursor, relative to the display (0 = top line of view, 1 = second line, etc.)
    // if limitToVisible is true, only lines which are currently visible will be searched for, and -1 returned if the line is not visible.
    int displayViewLine(const KTextEditor::Cursor& virtualCursor, bool limitToVisible = false) const;

    int viewCacheLineCount() const;
    KTextEditor::Cursor viewCacheStart() const;
    KTextEditor::Cursor viewCacheEnd() const;
    void updateViewCache(const KTextEditor::Cursor& startPos, int newViewLineCount = -1, int viewLinesScrolled = 0);

    // find the index of the last view line for a specific line
    int lastViewLine(int realLine) const;
    // find the view line of cursor c (0 = same line, 1 = down one, etc.)
    int viewLine(const KTextEditor::Cursor& realCursor) const;
    int viewLineCount(int realLine) const;

    void viewCacheDebugOutput() const;
    // END

    // Methods for invalidation of layouts
    // Methods for tagging layouts as dirty

private slots:
    void slotTextInserted(KTextEditor::Document *document, const KTextEditor::Range& range);
    void slotTextRemoved(KTextEditor::Document *document, const KTextEditor::Range& range);
    void slotTextChanged(KTextEditor::Document *document, const KTextEditor::Range& oldRange, const KTextEditor::Range& newRange);

private:
    void updateCache(int fromLine, int toLine, int shiftAmount);

    void expireUnused();
    void expireCount(int numberToExpire);

    KateRenderer* m_renderer;

    /**
     * The master cache of all line layouts.
     *
     * Layouts which are not within the current view cache and whose
     * refcount == 1 are only known to the cache and can be safely deleted.
     */
    mutable QMap<int, KateLineLayoutPtr> m_lineLayouts;

    // Convenience vector for quick direct access to the specific text layout
    KTextEditor::Cursor m_startPos;
    mutable QVector<KateTextLayout> m_textLayouts;

    int m_viewWidth;
    bool m_wrap;
};

#endif
