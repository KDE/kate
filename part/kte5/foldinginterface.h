/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2013 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KTEXTEDITOR_FOLDINGINTERFACE_H
#define KTEXTEDITOR_FOLDINGINTERFACE_H

// #include "ktexteditor_export.h"

#include <QFlags>

namespace KTextEditor {

class Range;

/**
 * KTextEditor interface for code folding of a KTextEditor::View.
 * The interface allows to arbitrary fold given regions of a buffer as long
 * as they are well nested.
 * Multiple instances of this class can exist for the same buffer.
 */
class KTEXTEDITOR_EXPORT FoldingInterface
{
  public:
    /**
     * Create folding object for given buffer.
     * @param buffer text buffer we want to provide folding info for
     */
    FoldingInterface ();

    /**
     * Cleanup
     */
    virtual ~FoldingInterface ();

    /**
     * Folding state of a range
     */
    enum FoldingRangeFlag {
      /**
       * Range is persistent, e.g. it should not auto-delete after unfolding!
       */
      Persistent = 0x1,

      /**
       * Range is folded away
       */
      Folded = 0x2
    };
    Q_DECLARE_FLAGS(FoldingRangeFlags, FoldingRangeFlag);

    /**
     * \name Creation, Folding and Unfolding
     *
     * The following functions provide access and manipulation of the range's position.
     * \{
     */

    /**
     * Create a new folding range.
     * @param range folding range
     * @param flags initial flags for the new folding range
     * @return on success, id of new range >= 0, else -1, we return no pointer as folding ranges might be auto-deleted internally!
     *         the ids are stable for one KTextEditor::FoldingInterface, e.g. you can rely in unit tests that you get 0,1,.... for successfully created ranges!
     */
    qint64 newFoldingRange (const KTextEditor::Range &range, FoldingRangeFlags flags = FoldingRangeFlags());

    /**
     * Fold the given range.
     * @param id id of the range to fold
     * @return success
     */
    virtual bool foldRange (qint64 id) = 0;

    /**
     * Unfold the given range.
     * In addition it can be forced to remove the region, even if it is persistent.
     * @param id id of the range to unfold
     * @param remove should the range be removed from the folding after unfolding? ranges that are not persistent auto-remove themself on unfolding
     * @return success
     */
    virtual bool unfoldRange (qint64 id, bool remove = false) = 0;

    /**
     * Queries which folding ranges start at the given line and returns the id + flags for all
     * of them. Very fast if nothing is folded, else binary search.
     * @param line line to query starting folding ranges
     * @return vector of id's + flags
     */
    virtual QVector<QPair<qint64, FoldingRangeFlags> > foldingRangesStartingOnLine (int line) const = 0;

    /**
     * Check whether on this line starts a folding range
     * @param line line to query starting folding ranges
     * @return true, if a folding range starts, otherwise false
     */
    virtual bool lineContainsStartFoldingRanges (int line) const = 0;

    /**
     * Fold the first folding range starting on this line, if applicable.
     * @param line line to fold
     * @return id of folded range (>= 0) or -1, if no folding range starts at line
     */
    virtual qint64 foldLine (int line) const = 0;

    /**
     * Unfolds all folding range starting on this line, if applicable.
     * @param line line to unfold
     * @return id of folded range (>= 0) or -1, if no folding range starts at line
     */
    virtual bool unfoldLine (int line) const = 0;

    /**
     * \}
     *
     * \name Line Visibility
     *
     * The following functions provide access and manipulation of the range's position.
     * \{
     */

    /**
     * Query if a given line is visible.
     * Very fast, if nothing is folded, else does binary search
     * log(n) for n == number of folded ranges
     * @param line line to query
     * @param foldedRangeId if the line is not visible and that pointer is not 0, will be filled with id of range hiding the line or -1
     * @return is that line visible?
     */
    virtual bool isLineVisible (int line, qint64 *foldedRangeId = 0) const = 0;

    /**
     * Ensure that a given line will be visible.
     * Potentially unfold recursively all folds hiding this line, else just returns.
     * @param line line to make visible
     */
    virtual void ensureLineIsVisible (int line) = 0;

    /**
     * Query number of visible lines.
     * Very fast, if nothing is folded, else walks over all folded regions
     * O(n) for n == number of folded ranges
     */
    virtual int visibleLines () const = 0;

    /**
     * Convert a text buffer line to a visible line number.
     * Very fast, if nothing is folded, else walks over all folded regions
     * O(n) for n == number of folded ranges
     * @param line line index in the text buffer
     * @return index in visible lines
     */
    virtual int lineToVisibleLine (int line) const = 0;

    /**
     * Convert a visible line number to a line number in the text buffer.
     * Very fast, if nothing is folded, else walks over all folded regions
     * O(n) for n == number of folded ranges
     * @param visibleLine visible line index
     * @return index in text buffer lines
     */
    virtual int visibleLineToLine (int visibleLine) const = 0;

    /**
     * \}
     */

  public Q_SLOTS:
    /**
     * Clear the complete folding.
     * This is automatically triggered if the buffer is cleared.
     */
    void clear ();

  Q_SIGNALS:
    /**
     * If the folding state of existing ranges changes or
     * ranges are added/removed, this signal is emitted.
     */
    void foldingRangesChanged ();
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(KTextEditor::FoldingInterface::FoldingRangeFlags)

#endif
