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

#ifndef KATE_TEXTFOLDING_H
#define KATE_TEXTFOLDING_H

#include "katepartprivate_export.h"

#include "ktexteditor/range.h"

#include <QObject>
#include <QFlags>
 
namespace Kate {

class TextBuffer;
class TextCursor;

/**
 * Class representing the folding information for a TextBuffer.
 * The interface allows to arbitrary fold given regions of a buffer as long
 * as they are well nested.
 * Multiple instances of this class can exist for the same buffer.
 */
class KATEPART_TESTS_EXPORT TextFolding : public QObject {
  Q_OBJECT
  
  public: 
    /**
     * Create folding object for given buffer.
     * @param buffer text buffer we want to provide folding info for
     */
    TextFolding (TextBuffer &buffer);
    
    /**
     * Cleanup
     */
    ~TextFolding ();

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
    Q_DECLARE_FLAGS(FoldingRangeFlags, FoldingRangeFlag)
    
    /**
     * Create a new folding range.
     * @param range folding range
     * @param flags initial flags for the new folding range
     * @return on success, id of new range >= 0, else -1, we return no pointer as folding ranges might be auto-deleted internally!
     *         the ids are stable for one Kate::TextFolding, e.g. you can rely in unit tests that you get 0,1,.... for successfully created ranges!
     */
    qint64 newFoldingRange (const KTextEditor::Range &range, FoldingRangeFlags flags = FoldingRangeFlags());

    /**
     * Returns the folding range associated with @p id.
     * If @p id is not a valid id, the returned range matches KTextEditor::Range::invalid().
     * @note This works for either persistend ranges or folded ranges.
     *       Note, that the highlighting does not add folds unless text is folded.
     *
     * @return the folding range for @p id
     */
    KTextEditor::Range foldingRange(qint64 id) const;

    /**
     * Fold the given range.
     * @param id id of the range to fold
     * @return success
     */
    bool foldRange (qint64 id);
    
    /**
     * Unfold the given range.
     * In addition it can be forced to remove the region, even if it is persistent.
     * @param id id of the range to unfold
     * @param remove should the range be removed from the folding after unfolding? ranges that are not persistent auto-remove themself on unfolding
     * @return success
     */
    bool unfoldRange (qint64 id, bool remove = false);
    
    /**
     * Query if a given line is visible.
     * Very fast, if nothing is folded, else does binary search
     * log(n) for n == number of folded ranges
     * @param line line to query
     * @param foldedRangeId if the line is not visible and that pointer is not 0, will be filled with id of range hiding the line or -1
     * @return is that line visible?
     */
    bool isLineVisible (int line, qint64 *foldedRangeId = 0) const;
    
    /**
     * Ensure that a given line will be visible.
     * Potentially unfold recursively all folds hiding this line, else just returns.
     * @param line line to make visible
     */
    void ensureLineIsVisible (int line);
    
    /**
     * Query number of visible lines.
     * Very fast, if nothing is folded, else walks over all folded regions
     * O(n) for n == number of folded ranges
     */
    int visibleLines () const;
    
    /**
     * Convert a text buffer line to a visible line number.
     * Very fast, if nothing is folded, else walks over all folded regions
     * O(n) for n == number of folded ranges
     * @param line line index in the text buffer
     * @return index in visible lines
     */
    int lineToVisibleLine (int line) const;
    
    /**
     * Convert a visible line number to a line number in the text buffer.
     * Very fast, if nothing is folded, else walks over all folded regions
     * O(n) for n == number of folded ranges
     * @param visibleLine visible line index
     * @return index in text buffer lines
     */
    int visibleLineToLine (int visibleLine) const;
    
    /**
     * Queries which folding ranges start at the given line and returns the id + flags for all
     * of them. Very fast if nothing is folded, else binary search.
     * @param line line to query starting folding ranges
     * @return vector of id's + flags
     */
    QVector<QPair<qint64, FoldingRangeFlags> > foldingRangesStartingOnLine (int line) const;
    
    /**
     * Return the current known folding ranges a QVariantList to store in configs.
     * @return current folds as variant list
     */
    QVariantList exportFoldingRanges () const;
    
    /**
     * Import the folding ranges given as a QVariantList like read from configs.
     * @param folds list of folds to import
     */
    void importFoldingRanges (const QVariantList &folds);
    
    /**
     * Dump folding state as string, for unit testing and debugging
     * @return current state as text
     */
    QString debugDump () const;
    
    /**
     * Print state to stdout for testing
     */
    void debugPrint (const QString &title) const;
  
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
      
  private:
    /**
     * Data holder for text folding range and its nested children
     */
    class FoldingRange {
      public:
        /**
         * Construct new one
         * @param buffer text buffer to use
         * @param range folding range
         * @param flags flags for the new folding range
         */
        FoldingRange (TextBuffer &buffer, const KTextEditor::Range &range, FoldingRangeFlags flags);
        
        /**
         * Cleanup
         */
        ~FoldingRange ();
        
        /**
         * Vector of range pointers
         */
        typedef QVector<FoldingRange*> Vector;
        
        /**
         * start moving cursor
         * NO range to be more efficient
         */
        Kate::TextCursor *start;
        
        /**
         * end moving cursor
         * NO range to be more efficient
         */
        Kate::TextCursor *end;
        
        /**
         * parent range, if any
         */
        FoldingRange *parent;
        
        /**
         * nested ranges, if any
         * this will always be sorted and non-overlapping
         * nested ranges are inside these ranges
         */
        FoldingRange::Vector nestedRanges;
        
        /**
         * Folding range flags
         */
        FoldingRangeFlags flags;
        
        /**
         * id of this range
         */
        qint64 id;
    };
    
    /**
     * Fill known folding ranges in a QVariantList to store in configs.
     * @param ranges ranges vector to dump
     * @param folds current folds as variant list, will be filled
     */
    static void exportFoldingRanges (const TextFolding::FoldingRange::Vector &ranges, QVariantList &folds);
    
    /**
     * Dump folding state of given vector as string, for unit testing and debugging.
     * Will recurse if wanted.
     * @param ranges ranges vector to dump
     * @param recurse recurse to nestedRanges?
     * @return current state as text
     */
    static QString debugDump (const TextFolding::FoldingRange::Vector &ranges, bool recurse);
    
    /**
     * Helper to insert folding range into existing ones.
     * Might fail, if not correctly nested.
     * Then the outside must take care of the passed pointer, e.g. delete it.
     * Will sanitize the ranges vectors, purge invalid/empty ranges.
     * @param parent parent folding range if any
     * @param existingRanges ranges into which we want to insert the new one
     * @param newRange new folding range
     * @return success, if false, newRange should be deleted afterwards, else it is registered internally
     */
    bool insertNewFoldingRange (FoldingRange *parent, TextFolding::FoldingRange::Vector &existingRanges, TextFolding::FoldingRange *newRange);
    
    /**
     * Helper to update the folded ranges if we insert a new range into the tree.
     * @param newRange new folding range that was inserted, will already contain its new nested ranges, if any!
     * @return any updated done? if yes, the foldingRangesChanged() signal got emitted!
     */
    bool updateFoldedRangesForNewRange (TextFolding::FoldingRange *newRange);
    
    /**
     * Helper to update the folded ranges if we remove a new range from the tree.
     * @param oldRange new folding range that is removed, will still contain its new nested ranges, if any!
     * @return any updated done? if yes, the foldingRangesChanged() signal got emitted!
     */
    bool updateFoldedRangesForRemovedRange (TextFolding::FoldingRange *oldRange);
    
    /**
     * Helper to append recursively topmost folded ranges from input to output vector.
     * @param newFoldedFoldingRanges output vector for folded ranges
     * @param ranges input vector to search recursively folded ranges inside
     */
    void appendFoldedRanges (TextFolding::FoldingRange::Vector &newFoldedFoldingRanges, const TextFolding::FoldingRange::Vector &ranges) const;
  
    /**
     * Compare two ranges by their start cursor.
     * @param a first range
     * @param b second range
     */
    static bool compareRangeByStart (FoldingRange *a, FoldingRange *b);

    /**
     * Compare two ranges by their end cursor.
     * @param a first range
     * @param b second range
     */
    static bool compareRangeByEnd (FoldingRange *a, FoldingRange *b);
    
    /**
     * Compare range start with line
     * @param line line
     * @param range range
     */
    static bool compareRangeByStartWithLine (int line, FoldingRange *range);
    
    /**
     * Compare range start with line
     * @param range range
     * @param line line
     */
    static bool compareRangeByLineWithStart (FoldingRange *range, int line);
    
    /**
     * Internal helper that queries which folding ranges start at the given line and returns the id + flags for all
     * of them. Will recursively dive down starting with given vector
     * @param results vector that is filled with id's + flags
     * @param ranges ranges vector to search in
     * @param line line to query starting folding ranges
     */
    void foldingRangesStartingOnLine (QVector<QPair<qint64, FoldingRangeFlags> > &results, const TextFolding::FoldingRange::Vector &ranges, int line) const;
    
  private:
    /**
     * parent text buffer
     * is a reference, and no pointer, as this must always exist and can't change
     * can't be const, as we create text cursors!
     */
    TextBuffer &m_buffer;
    
    /**
     * toplevel folding ranges
     * this will always be sorted and non-overlapping
     * nested ranges are inside these ranges
     */
    FoldingRange::Vector m_foldingRanges;

    /**
     * folded folding ranges
     * this is a sorted vector of ranges
     * all non-overlapping
     */
    FoldingRange::Vector m_foldedFoldingRanges;
    
    /**
     * global id counter for the created ranges
     */
    qint64 m_idCounter;
    
    /**
     * mapping: id => range
     */
    QHash<qint64, FoldingRange *> m_idToFoldingRange;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Kate::TextFolding::FoldingRangeFlags)

#endif
