/* This file is part of the KDE project
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2001-2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef KDELIBS_KTEXTEDITOR_RANGE_H
#define KDELIBS_KTEXTEDITOR_RANGE_H

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/cursor.h>


namespace KTextEditor
{
class SmartRange;

/**
 * \short An object representing a section of text, from one Cursor to another.
 *
 * A Range is a basic class which represents a range of text with two Cursors,
 * from a start() position to an end() position.
 *
 * For simplicity and convenience, ranges always maintain their start position to
 * be before or equal to their end position.  Attempting to set either the
 * start or end of the range beyond the respective end or start will result in
 * both values being set to the specified position.  In the constructor, the
 * start and end will be swapped if necessary.
 *
 * If you want additional functionality such as the ability to maintain position
 * in a document, see SmartRange.
 *
 * \sa SmartRange
 *
 * \author Hamish Rodda \<rodda@kde.org\>
 */
class KTEXTEDITOR_EXPORT Range
{
  friend class Cursor;

  public:
    /**
     * Default constructor. Creates a valid range from position (0, 0) to
     * position (0, 0).
     */
    Range();

    /**
     * Constructor which creates a range from \e start to \e end.
     * If start is after end, they will be swapped.
     *
     * \param start start position
     * \param end end position
     */
    Range(const Cursor& start, const Cursor& end);

    /**
     * Constructor which creates a single-line range from \p start,
     * extending \p width characters along the same line.
     *
     * \param start start position
     * \param width width of this range in columns along the same line
     */
    Range(const Cursor& start, int width);

    /**
     * Constructor which creates a range from \p start, to \p endLine, \p endColumn.
     *
     * \param start start position
     * \param endLine end line
     * \param endColumn end column
     */
    Range(const Cursor& start, int endLine, int endColumn);

    /**
     * Constructor which creates a range from \e startLine, \e startColumn to \e endLine, \e endColumn.
     *
     * \param startLine start line
     * \param startColumn start column
     * \param endLine end line
     * \param endColumn end column
     */
    Range(int startLine, int startColumn, int endLine, int endColumn);

    /**
     * Copy constructor.
     *
     * \param copy the range from which to copy the start and end position.
     */
    Range(const Range& copy);

    /**
     * Virtual destructor.
     */
    //Do not remove! Needed for inheritance.
    virtual ~Range();

    /**
     * Validity check.  In the base class, returns true unless the range starts before (0,0).
     */
    virtual bool isValid() const;

    /**
     * Returns an invalid range.
     */
    static Range invalid();

    /**
     * Returns whether this range is a SmartRange.
     */
    virtual bool isSmartRange() const;

    /**
     * Returns this range as a SmartRange, if it is one.
     */
    virtual SmartRange* toSmartRange() const;

    /**
     * \name Position
     *
     * The following functions provide access to, and manipulation of, the range's position.
     * \{
     */
    /**
     * Get the start position of this range. This will always be <= end().
     *
     * This non-const function allows direct manipulation of the start position,
     * while still retaining notification support.
     *
     * If start is set to a position after end, end will be moved to the
     * same position as start, as ranges are not allowed to have
     * start() > end().
     *
     * \note If you want to change both start() and end() simultaneously,
     *       you should use setRange(), for several reasons:
     *       \li otherwise, the rule preventing start() > end() may alter your intended change
     *       \li any notifications needed will be performed multiple times for no benefit
     *
     * \returns a reference to the start position of this range.
     */
    Cursor& start();

    /**
     * Get the start position of this range. This will always be <= end().
     *
     * \returns a const reference to the start position of this range.
     *
     * \internal this function is virtual to allow for covariant return of SmartCursor%s.
     */
    const Cursor& start() const;

    /**
     * Get the end position of this range. This will always be >= start().
     *
     * This non-const function allows direct manipulation of the end position,
     * while still retaining notification support.
     *
     * If end is set to a position before start, start will be moved to the
     * same position as end, as ranges are not allowed to have
     * start() > end().
     *
     * \note If you want to change both start() and end() simultaneously,
     *       you should use setRange(), for several reasons:
     *       \li otherwise, the rule preventing start() > end() may alter your intended change
     *       \li any notifications needed will be performed multiple times for no benefit
     *
     * \returns a reference to the end position of this range.
     *
     * \internal this function is virtual to allow for covariant return of SmartCursor%s.
     */
    Cursor& end();

    /**
     * Get the end position of this range. This will always be >= start().
     *
     * \returns a const reference to the end position of this range.
     *
     * \internal this function is virtual to allow for covariant return of SmartCursor%s.
     */
    const Cursor& end() const;

    /**
     * Convenience function.  Set the start and end lines to \p line.
     *
     * \param line the line number to assign to start() and end()
     */
    void setBothLines(int line);

    /**
     * Convenience function.  Set the start and end columns to \p column.
     *
     * \param column the column number to assign to start() and end()
     */
    void setBothColumns(int column);

    /**
     * Set the start and end cursors to \e range.start() and \e range.end() respectively.
     *
     * \param range range to assign to this range
     */
    virtual void setRange(const Range& range);

    /**
     * \overload
     * \n \n
     * Set the start and end cursors to \e start and \e end respectively.
     *
     * \note If \e start is after \e end, they will be reversed.
     *
     * \param start start cursor
     * \param end end cursor
     */
    void setRange(const Cursor& start, const Cursor& end);

    /**
     * Expand this range if necessary to contain \p range.
     *
     * \param range range which this range should contain
     *
     * \return \e true if expansion occurred, \e false otherwise
     */
    virtual bool expandToRange(const Range& range);

    /**
     * Confine this range if necessary to fit within \p range.
     *
     * \param range range which should contain this range
     *
     * \return \e true if confinement occurred, \e false otherwise
     */
    virtual bool confineToRange(const Range& range);

    /**
     * Check whether this range is wholly contained within one line, ie. if
     * the start() and end() positions are on the same line.
     *
     * \return \e true if both the start and end positions are on the same
     *         line, otherwise \e false
     */
    bool onSingleLine() const;

    /**
     * Returns the number of lines separating the start() and end() positions.
     *
     * \return the number of lines separating the start() and end() positions;
     *         0 if the start and end lines are the same.
     */
    int numberOfLines() const;

    /**
     * Returns the number of columns separating the start() and end() positions.
     *
     * \return the number of columns separating the start() and end() positions;
     *         0 if the start and end columns are the same.
     */
    int columnWidth() const;

    /**
     * Returns true if this range contains no characters, ie. the start() and
     * end() positions are the same.
     *
     * \returns \e true if the range contains no characters, otherwise \e false
     */
    bool isEmpty() const;

    //BEGIN comparison functions
    /**
     * \}
     *
     * \name Comparison
     *
     * The following functions perform checks against this range in comparison
     * to other lines, columns, cursors, and ranges.
     * \{
     */
    /**
     * Check whether the this range wholly encompasses \e range.
     *
     * \param range range to check
     *
     * \return \e true, if this range contains \e range, otherwise \e false
     */
    bool contains(const Range& range) const;

    /**
     * Check to see if \p cursor is contained within this range, ie >= start() and \< end().
     *
     * \param cursor the position to test for containment
     *
     * \return \e true if the cursor is contained within this range, otherwise \e false.
     */
    bool contains(const Cursor& cursor) const;

    /**
     * Returns true if this range wholly encompasses \p line.
     *
     * \param line line to check
     *
     * \return \e true if the line is wholly encompassed by this range, otherwise \e false.
     */
    bool containsLine(int line) const;

    /**
     * Check whether the range contains \e column.
     *
     * \param column column to check
     *
     * \return \e true if the range contains \e column, otherwise \e false
     */
    bool containsColumn(int column) const;

    /**
     * Check whether the this range overlaps with \e range.
     *
     * \param range range to check against
     *
     * \return \e true, if this range overlaps with \e range, otherwise \e false
     */
    bool overlaps(const Range& range) const;

    /**
     * Check whether the range overlaps at least part of \e line.
     *
     * \param line line to check
     *
     * \return \e true, if the range overlaps at least part of \e line, otherwise \e false
     */
    bool overlapsLine(int line) const;

    /**
     * Check to see if this range overlaps \p column; that is, if \p column is
     * between start().column() and end().column().  This function is most likely
     * to be useful in relation to block text editing.
     *
     * \param column the column to test
     *
     * \return \e true if the column is between the range's starting and ending
     *         columns, otherwise \e false.
     */
    bool overlapsColumn(int column) const;

    /**
     * Determine where \p cursor is positioned in relationship to this range.
     * Equivalency (a return value of 0) is returned when \p cursor is \e contained
     * within the range, not when \e overlapped - i.e., \p cursor may be on a
     * line which is also partially occupied by this range, but the position
     * may not be eqivalent.  For overlap checking, use positionRelativeToLine().
     *
     * \param cursor position to check
     *
     * \return \e -1 if before, \e +1 if after, and \e 0 if \p cursor is contained within the range.
     *
     * \see positionRelativeToLine()
     */
    int positionRelativeToCursor(const Cursor& cursor) const;

    /**
     * Determine where \p line is positioned in relationship to this range.
     * Equivalency (a return value of 0) is returned when \p line is \e overlapped
     * within the range, not when \e contained - i.e., this range may not cover an entire line,
     * but \p line's position will still be eqivalent.  For containment checking, use positionRelativeToCursor().
     *
     * \param line line to check
     *
     * \return \e -1 if before, \e +1 if after, and \e 0 if \p line is overlapped by this range.
     *
     * \see positionRelativeToCursor()
     */
    int positionRelativeToLine(int line) const;

    /**
     * Check whether \p cursor is located at either of the start() or end()
     * boundaries.
     *
     * \param cursor cursor to check
     *
     * \return \e true if the cursor is equal to \p start() or \p end(),
     *         otherwise \e false.
     */
    bool boundaryAtCursor(const Cursor& cursor) const;

    /**
     * Check whether \p line is on the same line as either of the start() or
     * end() boundaries.
     *
     * \param line line to check
     *
     * \return \e true if \p line is on the same line as either of the
     *         boundaries, otherwise \e false
     */
    bool boundaryOnLine(int line) const;

    /**
     * Check whether \p column is on the same column as either of the start()
     * or end() boundaries.
     *
     * \param column column to check
     *
     * \return \e true if \p column is on the same column as either of the
     *         boundaries, otherwise \e false
     */
    bool boundaryOnColumn(int column) const;
    //!\}
    //END

    /**
     * Intersects this range with another, returning the shared area of
     * the two ranges.
     *
     * \param range other range to intersect with this
     *
     * \return the intersection of this range and the supplied \a range.
     */
    Range intersect(const Range& range) const;

    /**
     * Returns the smallest range which encompasses this range and the
     * supplied \a range.
     *
     * \param range other range to encompass
     *
     * \return the smallest range which contains this range and the supplied \a range.
     */
    Range encompass(const Range& range) const;

    /**
     * Assignment operator. Same as setRange().
     *
     * \param rhs range to assign to this range.
     *
     * \return a reference to this range, after assignment has occurred.
     *
     * \see setRange()
     */
    inline Range& operator=(const Range& rhs)
      { setRange(rhs); return *this; }

    /**
     * Addition operator. Takes two ranges and returns their summation.
     *
     * \param r1 the first range
     * \param r2 the second range
     *
     * \return a the summation of the two input ranges
     */
    inline friend Range operator+(const Range& r1, const Range& r2)
      { return Range(r1.start() + r2.start(), r1.end() + r2.end()); }

    /**
     * Addition assignment operator. Adds \p r2 to this range.
     *
     * \param r1 the first range
     * \param r2 the second range
     *
     * \return a reference to the cursor which has just been added to
     */
    inline friend Range& operator+=(Range& r1, const Range& r2)
      { r1.setRange(r1.start() + r2.start(), r1.end() + r2.end()); return r1; }

    /**
     * Subtraction operator. Takes two ranges and returns the subtraction
     * of \p r2 from \p r1.
     *
     * \param r1 the first range
     * \param r2 the second range
     *
     * \return a range representing the subtraction of \p r2 from \p r1
     */
    inline friend Range operator-(const Range& r1, const Range& r2)
      { return Range(r1.start() - r2.start(), r1.end() - r2.end()); }

    /**
     * Subtraction assignment operator. Subtracts \p r2 from \p r1.
     *
     * \param r1 the first range
     * \param r2 the second range
     *
     * \return a reference to the range which has just been subtracted from
     */
    inline friend Range& operator-=(Range& r1, const Range& r2)
      { r1.setRange(r1.start() - r2.start(), r1.end() - r2.end()); return r1; }

    /**
     * Intersects \a r1 and \a r2.
     *
     * \param r1 the first range
     * \param r2 the second range
     *
     * \return the intersected range, invalid() if there is no overlap
     */
    inline friend Range operator&(const Range& r1, const Range& r2)
      { return r1.intersect(r2); }

    /**
     * Intersects \a r1 with \a r2 and assigns the result to \a r1.
     *
     * \param r1 the range to assign the intersection to
     * \param r2 the range to intersect \a r1 with
     *
     * \return a reference to this range, after the intersection has taken place
     */
    inline friend Range& operator&=(Range& r1, const Range& r2)
      { r1.setRange(r1.intersect(r2)); return r1; }

    /**
     * Equality operator.
     *
     * \param r1 first range to compare
     * \param r2 second range to compare
     *
     * \return \e true if \e r1 and \e r2 equal, otherwise \e false
     */
    inline friend bool operator==(const Range& r1, const Range& r2)
      { return r1.start() == r2.start() && r1.end() == r2.end(); }

    /**
     * Inequality operator.
     *
     * \param r1 first range to compare
     * \param r2 second range to compare
     *
     * \return \e true if \e r1 and \e r2 do \e not equal, otherwise \e false
     */
    inline friend bool operator!=(const Range& r1, const Range& r2)
      { return r1.start() != r2.start() || r1.end() != r2.end(); }

    /**
     * Greater than operator.  Looks only at the position of the two ranges,
     * does not consider their size.
     *
     * \param r1 first range to compare
     * \param r2 second range to compare
     *
     * \return \e true if \e r1 starts after where \e r2 ends, otherwise \e false
     */
    inline friend bool operator>(const Range& r1, const Range& r2)
      { return r1.start() > r2.end(); }

    /**
     * Less than operator.  Looks only at the position of the two ranges,
     * does not consider their size.
     *
     * \param r1 first range to compare
     * \param r2 second range to compare
     *
     * \return \e true if \e r1 ends before \e r2 begins, otherwise \e false
     */
    inline friend bool operator<(const Range& r1, const Range& r2)
      { return r1.end() < r2.start(); }

    /**
     * kDebug() stream operator.  Writes this range to the debug output in a nicely formatted way.
     */
    inline friend QDebug operator<< (QDebug s, const Range& range) {
      if (&range)
        s << "[" << range.start() << " -> " << range.end() << "]";
      else
        s << "(null range)";
      return s;
    }

  protected:
    /**
     * Constructor for advanced cursor types.
     * Creates a range from \e start to \e end.
     * Takes ownership of \e start and \e end.
     *
     * \param start the start cursor.
     * \param end the end cursor.
     */
    Range(Cursor* start, Cursor* end);

    /**
     * Notify this range that one or both of the cursors' position has changed directly.
     *
     * \param cursor the cursor that changed. If 0L, both cursors have changed.
     * \param from the previous position of this range
     *
     * \internal
     */
    virtual void rangeChanged(Cursor* cursor, const Range& from);

    /**
     * This range's start cursor pointer.
     *
     * \internal
     */
    Cursor* m_start;

    /**
     * This range's end cursor pointer.
     *
     * \internal
     */
    Cursor* m_end;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
