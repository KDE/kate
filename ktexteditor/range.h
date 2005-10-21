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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_RANGE_H
#define KDELIBS_KTEXTEDITOR_RANGE_H

#include <kdelibs_export.h>
#include <ktexteditor/cursor.h>

class KAction;

namespace KTextEditor
{
class Attribute;

/**
 * \short A Range represents a section of text, from one Cursor to another.
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
 * If you want additional functionality such as the ability to maintain positon
 * in a document, see SmartRange.
 *
 * \sa SmartRange
 */
class KTEXTEDITOR_EXPORT Range
{
  friend class Cursor;

  public:
    /**
     * The default constructor creates a range from position (0, 0) to
     * position (0, 0).
     */
    Range();

    /**
     * Constructor.
     * Creates a range from @e start to @e end.
     * If start is after end, they will be swapped.
     * @param start start position
     * @param end end position
     */
    Range(const Cursor& start, const Cursor& end);

    /**
     * Constructor
     * Creates a single-line range from \p start which extends \p width characters along the same line.
     */
    Range(const Cursor& start, int width);

    /**
     * Constructor
     * Creates a range from \p start to \p endLine, \p endCol.
     */
    Range(const Cursor& start, int endLine, int endCol);

    /**
     * Constructor.
     * Creates a range from @e startLine, @e startCol to @e endLine, @e endCol.
     * @param startLine start line
     * @param startCol start column
     * @param endLine end line
     * @param endCol end column
     */
    Range(int startLine, int startCol, int endLine, int endCol);

    /**
     * Copy constructor
     */
    Range(const Range& copy);

    /**
     * Virtual destructor
     */
    virtual ~Range();

    /**
     * Validity check.  In the base class, returns true unless the range starts before (0,0).
     */
    virtual bool isValid() const;

    /**
     * Returns an invalid range.
     */
    static const Range& invalid();

    /**
     * Get the start point of this range. This will always be <= end().
     *
     * This non-const function allows direct manipulation of start(), while still retaining
     * notification support.
     *
     * If start() is set to a position after end(), end() will be moved to the
     * same position as start(), as ranges are not allowed to have
     * start() > end().
     */
    inline Cursor& start() { return *m_start; }

    /**
     * Get the start point of this range. This will always be <= end().
     */
    inline const Cursor& start() const { return *m_start; }

    /**
     * Get the end point of this range. This will always be >= start().
     *
     * This non-const function allows direct manipulation of end(), while still retaining
     * notification support.
     *
     * If end() is set to a position before start(), start() will be moved to the
     * same position as end(), as ranges are not allowed to have
     * start() > end().
     */
    inline Cursor& end() { return *m_end; }

    /**
     * Get the end point of this range. This will always be >= start().
     */
    inline const Cursor& end() const { return *m_end; }

    /**
     * Convenience function.  Set the start and end lines to \p line.
     *
     * \param line the line number to assign to start() and end()
     */
    inline void setBothLines(int line) { setRange(Range(line, start().column(), line, end().column())); }

    /**
     * Set the start and end cursors to @e range.
     * @param range new range
     */
    virtual void setRange(const Range& range);

    /**
     * @overload void setRange(const Range& range)
     * If @e start is after @e end, they will be reversed.
     * @param start start cursor
     * @param end end cursor
     */
    void setRange(const Cursor& start, const Cursor& end);

    /**
     * Expand this range if necessary to contain \p range.
     *
     * \param range range which this range should contain
     * \return true if expansion occurred, false otherwise
     */
    virtual bool expandToRange(const Range& range);

    /**
     * Confine this range if neccessary to fit within \p range.
     *
     * \param range range which should contain this range
     * \return true if confinement occurred, false otherwise
     */
    virtual bool confineToRange(const Range& range);

    // TODO: produce int versions with -1 before, 0 true, and +1 after if there is a need
    /**
     * Returns true if this range wholly encompasses \p line.
     */
    bool containsLine(int line) const;

    /**
     * Check whether the range includes at least part of @e line.
     * @param line line to check
     * @return @e true, if the range includes at least part of @e line, otherwise @e false
     */
    bool includesLine(int line) const;

    /**
     * Returns true if this range spans \p colmun.
     */
    bool spansColumn(int column) const;

    /**
     * Returns true if \p cursor is wholly contained within this range, ie >= start() and \< end().
     * \param cursor Cursor to test for containment
     */
    bool contains(const Cursor& cursor) const;

    /**
     * Check whether the range includes @e column.
     * @param column column to check
     * @return @e true, if the range includes @e column, otherwise @e false
     * \todo should be contains?
     */
    bool includesColumn(int column) const;
    /**
     * Check whether the range includes @e cursor. Returns
     * - -1 if @e cursor < @p start()
     * - 0 if @p start() <= @e cursor <= @p end()
     * - 1 if @e cursor > @p end()
     * @param line line to check
     * @return depending on the case either -1, 0 or 1
     * \todo should be contains?
     */
    int includes(const Cursor& cursor) const;

    /**
     * Check whether the this range contains @e range.
     * @param range range to check
     * @return @e true, if this range contains @e range, otherwise @e false
     */
    bool contains(const Range& range) const;
    /**
     * Check whether the this range overlaps with @e range.
     * @param range range to check against
     * @return @e true, if this range overlaps with @e range, otherwise @e false
     */
    bool overlaps(const Range& range) const;
    /**
     * Check whether @e cursor == @p start() or @e cursor == @p end().
     * @param cursor cursor to check
     * @return @e true, if the cursor is equal to @p start() or @p end(),
     *         otherwise @e false
     */
    bool boundaryAt(const Cursor& cursor) const;
    /**
     * Check whether @e line == @p start().line() or @e line == @p end().line().
     * @param line line to check
     * @return @e true, if the line is either the same with the start bound
     *         or the end bound, otherwise @e false
     */
    bool boundaryOnLine(int line) const;
    /**
     * Check whether @e column == @p start().column() or @e column == @p end().column().
     * @param column column to check
     * @return @e true, if the column is either the same with the start bound
     *         or the end bound, otherwise @e false
     */
    bool boundaryOnColumn(int column) const;

    /**
     * Check whether this range is wholly contained within one line, ie. if
     * start().line() == end().line().
     */
    inline bool onSingleLine() const { return start().line() == end().line(); }

    /**
     * Returns where \p cursor is positioned, relative to this range.
     * @return \e -1 if before, \e +1 if after, and \e 0 if \p cursor is contained within the range.
     */
    inline int relativePosition(const Cursor& cursor) const
      { return ((cursor < start()) ? -1 : ((cursor > end()) ? 1:0)); }

    /**
     * Returns the number of columns of the end() relative to the start().
     */
    inline int columnWidth() const { return end().column() - start().column(); }

    /**
     * Returns true if this range contains no characters, ie. the start() and end() positions are the same.
     */
    inline bool isEmpty() const { return start() == end(); }

    /**
     * = operator. Assignment.
     * @param rhs new range
     * @return *this
     */
    virtual Range& operator= (const Range& rhs);

    inline friend Range operator+(const Range& r1, const Range& r2) { return Range(r1.start() + r2.start(), r1.end() + r2.end()); }
    inline friend Range& operator+=(Range& r1, const Range& r2) { r1.setRange(r1.start() + r2.start(), r1.end() + r2.end()); return r1; }

    inline friend Range operator-(const Range& r1, const Range& r2) { return Range(r1.start() - r2.start(), r1.end() - r2.end()); }
    inline friend Range& operator-=(Range& r1, const Range& r2) { r1.setRange(r1.start() - r2.start(), r1.end() - r2.end()); return r1; }

    /**
     * == operator
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return @e true, if @e r1 and @e r2 equal, otherwise @e false
     */
    inline friend bool operator==(const Range& r1, const Range& r2)
      { return r1.start() == r2.start() && r1.end() == r2.end(); }

    /**
     * != operator
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return @e true, if @e r1 and @e r2 do @e not equal, otherwise @e false
     */
    inline friend bool operator!=(const Range& r1, const Range& r2)
      { return r1.start() != r2.start() || r1.end() != r2.end(); }

    /**
     * Greater than operator.
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return @e true, if @e r1 starts after where @e r2 ends, otherwise @e false
     */
    inline friend bool operator>(const Range& r1, const Range& r2)
      { return r1.start() > r2.end(); }

    /**
     * Less than operator.
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return @e true, if @e r1 ends before @e r2 begins, otherwise @e false
     */
    inline friend bool operator<(const Range& r1, const Range& r2)
      { return r1.end() < r2.start(); }

    inline friend kdbgstream& operator<< (kdbgstream& s, const Range& range) {
      if (&range)
        s << "[" << range.start() << " -> " << range.end() << "]";
      else
        s << "(null range)";
      return s;
    }

    inline friend kndbgstream& operator<< (kndbgstream& s, const Range&) { return s; }

  protected:
    /**
     * Constructor for advanced cursor types.
     * Creates a range from @e start to @e end.
     * Takes ownership of @e start and @e end.
     */
    Range(Cursor* start, Cursor* end);

    enum {
      RangeStartExpanded = 0x1,
      RangeStartContracted = 0x2,
      RangeEndExpanded = 0x4,
      RangeEndContracted = 0x8
    };

    /**
     * \internal
     *
     * Notify this range that one or both of the cursors' position has changed directly.
     *
     * \param cursor the cursor that changed. If 0L, both cursors have changed.
     * \param from the previous position of this range
     */
    virtual void rangeChanged(Cursor* cursor, const Range& from);

    Cursor* m_start;
    Cursor* m_end;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
