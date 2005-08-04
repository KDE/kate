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

#ifndef __ktexteditor_range_h__
#define __ktexteditor_range_h__

#include <kdelibs_export.h>
#include <ktexteditor/cursor.h>

namespace KTextEditor
{

/**
 * The class @p Range represents a range in the text from one @p Cursor to
 * another.
 *
 * For simplicity and convenience reason, ranges always maintain their start
 * position to be before or equal to their end position.
 */
class KTEXTEDITOR_EXPORT Range
{
  public:
    /**
     * The default constructor creates a range from position (0,0) to
     * position (0,0).
     */
    Range();

    /**
     * Constructor.
     * Creates a range from @e start to @e end.
     * @param start start position
     * @param end end position
     */
    Range(const Cursor& start, const Cursor& end);

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
     * Virtual destructor
     */
    virtual ~Range();

    /**
     * Get the start point of this range. This will always be >= start().
     */
    inline const Cursor& start() const { return *m_start; }

    /**
     * Set the start point of this range. If @e start is after @p end(),
     * @p end() will be moved to the same point as @e start, as ranges are
     * not allowed to have @p start() > @p end().
     * @param start start cursor
     */
    virtual void setStart(const Cursor& start);
    /**
     * @overload void setStart(const Cursor& start)
     * @param line start line
     * @param column start column
     */
    inline void setStart(int line, int column) { setStart(Cursor(line, column)); }
    /**
     * @overload void setStart(const Cursor& start)
     * @param line start line
     */
    inline void setStartLine(int line) { setStart(Cursor(line, start().column())); }
    /**
     * @overload void setStart(const Cursor& start)
     * @param column start column
     */
    inline void setStartColumn(int column) { setStart(Cursor(start().line(), column)); }

    /**
     * Get the end point of this range. This will always be >= @p start().
     */
    inline const Cursor& end() const { return *m_end; }

    /**
     * Set the end point of this range. If @e end is before @p start(),
     * @p start() will be moved to the same point as @e end, as ranges are
     * not allowed to have @p start() > @p end().
     * @param end end cursor
     */
    virtual void setEnd(const Cursor& end);
    /**
     * @overload void setEnd(const Cursor& end)
     * @param line end line
     * @param column end column
     */
    inline void setEnd(int line, int column) { setEnd(Cursor(line, column)); }
    /**
     * @overload void setEnd(const Cursor& end)
     * @param line end line
     */
    inline void setEndLine(int line) { setEnd(Cursor(line, end().column())); }
    /**
     * @overload void setEnd(const Cursor& end)
     * @param column end column
     */
    inline void setEndColumn(int column) { setEnd(Cursor(end().line(), column)); }

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
    virtual void setRange(const Cursor& start, const Cursor& end);

    /**
     * Check whether the range includes @e line.
     * @param line line to check
     * @return @e true, if the range includes @e line, otherwise @e false
     */
    bool includesLine(int line) const;
    /**
     * Check whether the range includes @e column.
     * @param column column to check
     * @return @e true, if the range includes @e column, otherwise @e false
     */
    bool includesColumn(int column) const;
    /**
     * Check whether the range includes @e cursor. Returns
     * - -1 if @e cursor < @p start()
     * - 0 if @p start() <= @e cursor <= @p end()
     * - 1 if @e cursor > @p end()
     * @param line line to check
     * @return depending on the case either -1, 0 or 1
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
     * = operator. Assignment.
     * @param rhs new range
     * @return *this
     */
    virtual Range& operator= (const Range& rhs) { setRange(rhs); return *this; }

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

  protected:
    /**
     * Constructor for advanced cursor types.
     * Creates a range from @e start to @e end.
     * Takes ownership of @e start and @e end.
     */
    Range(Cursor* start, Cursor* end);

    Cursor* m_start;
    Cursor* m_end;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
