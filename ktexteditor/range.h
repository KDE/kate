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
 * Class to represent a range in the text from one Cursor to another.
 *
 * For simplicity, ranges always maintain their start position to
 * be before or equal to their end position.
 */
class KTEXTEDITOR_EXPORT Range
{
  public:
    /**
     * Default constructor
     * Creates a range from position 0,0 to position 0,0
     */
    Range();

    /**
     * Constructor
     * Creates a range from \p start to \p end
     */
    Range(const Cursor& start, const Cursor& end);

    /**
     * Constructor
     * Creates a range from \p startLine, \p startCol to \p endLine, \p endCol.
     */
    Range(int startLine, int startCol, int endLine, int endCol);

    /**
     * Virtual destructor
     */
    virtual ~Range();

    /**
     * Returns the start point of this range.  This will always be >= start().
     */
    inline const Cursor& start() const { return *m_start; }

    /**
     * Sets the starting point of this range.  If \p start is after end(),
     * end() will be moved to the same point as \p start, as ranges are not
     * allowed to have start() > end().
     */
    virtual void setStart(const Cursor& start);
    inline void setStart(int line, int column) { setStart(Cursor(line, column)); }
    inline void setStartLine(int line) { setStart(Cursor(line, start().column())); }
    inline void setStartColumn(int column) { setStart(Cursor(start().line(), column)); }

    /**
     * Returns the end point of this range.  This will always be >= start().
     */
    inline const Cursor& end() const { return *m_end; }

    /**
     * Sets the end point of this range.  If \p end is before start(),
     * start() will be moved to the same point as \p end, as ranges are not
     * allowed to have start() > end().
     */
    virtual void setEnd(const Cursor& end);
    inline void setEnd(int line, int column) { setEnd(Cursor(line, column)); }
    inline void setEndLine(int line) { setEnd(Cursor(line, end().column())); }
    inline void setEndColumn(int column) { setEnd(Cursor(end().line(), column)); }

    /**
     * Sets the start and end points of this range.
     */
    virtual void setRange(const Range& range);

    /**
     * Sets the start and end points of this range.  If \p start is after
     * \p end, they will be reversed.
     */
    virtual void setRange(const Cursor& start, const Cursor& end);

    bool includesLine(int line) const;
    bool includesColumn(int column) const;
    int includes(const Cursor& cursor) const;
    bool contains(const Range& range) const;
    bool overlaps(const Range& range) const;
    bool boundaryAt(const Cursor& cursor) const;
    bool boundaryOnLine(int line) const;
    bool boundaryOnColumn(int column) const;

    virtual Range& operator= (const Range& rhs) { setRange(rhs); return *this; }

    /**
     * == operator
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return result of compare
     */
    inline friend bool operator==(const Range& r1, const Range& r2)
      { return r1.start() == r2.start() && r1.end() == r2.end(); }

    /**
     * != operator
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return result of compare
     */
    inline friend bool operator!=(const Range& r1, const Range& r2)
      { return r1.start() != r2.start() || r1.end() != r2.end(); }

    /**
     * Greater than operator.
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return true if \p r1 starts after \p r2 ends.
     */
    inline friend bool operator>(const Range& r1, const Range& r2)
      { return r1.start() > r2.end(); }

    /**
     * Less than operator.
     * @param r1 first range to compare
     * @param r2 second range to compare
     * @return true if \p r1 ends before \p r2 begins.
     */
    inline friend bool operator<(const Range& r1, const Range& r2)
      { return r1.end() < r2.start(); }

  protected:
    /**
     * Constructor for advanced cursor types.
     * Creates a range from \p start to \p end.
     * Takes ownership of \p start and \p end.
     */
    Range(Cursor* start, Cursor* end);

    Cursor* m_start;
    Cursor* m_end;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
