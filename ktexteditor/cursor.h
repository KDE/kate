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

#ifndef KDELIBS_KTEXTEDITOR_CURSOR_H
#define KDELIBS_KTEXTEDITOR_CURSOR_H

#include <kdelibs_export.h>
#include <kdebug.h>

namespace KTextEditor
{
class Document;
class Range;

/**
 * \short A Cursor represents a position in a Document.
 *
 * A Cursor is a basic class which contains the line() and column() a position
 * in a Document. It is very lightweight and maintains no affiliation with a
 * particular Document.
 *
 * \note The Cursor class is designed to be passed via value, while SmartCursor
 * and derivatives must be passed via pointer as they maintain a connection
 * with their document internally.
 * \note Lines and columns start at 0.
 *
 * If you want additional functionality such as the ability to maintain positon
 * in a document, see SmartCursor.
 *
 * \sa SmartCursor
 */
class KTEXTEDITOR_EXPORT Cursor
{
  friend class Range;

  public:
    /**
     * The default constructor creates a cursor at position (0,0).
     */
    Cursor();

    /**
     * This constructor creates a cursor initialized with @e line
     * and @e column.
     * @param line line for cursor
     * @param column column for cursor
     */
    Cursor(int line, int column);

    /// Copy constructor.
    Cursor(const Cursor& copy);

    // Virtual destructor. Do not remove! Needed for inheritance.
    virtual ~Cursor ();

    /**
     * Returns whether this cursor is a SmartCursor.
     */
    virtual bool isSmart() const;

    /**
     * Returns whether the current position of this cursor is a valid position
     * (line + column must both be >= 0).
     *
     * Smart cursors should override this to return whether the cursor is valid
     * within the linked document.
     */
    virtual bool isValid() const;

    /**
     * Returns an invalid cursor.
     * \todo should invalid cursors be always or never equal to each other?
     */
    static const Cursor& invalid();

    /**
     * Returns a cursor representing the start of any document - i.e., line 0, column 0.
     */
    static const Cursor& start();

    /**
     * Get both the line and column of the cursor position.
     * @param _line will be filled with current cursor line
     * @param _column will be filled with current cursor column
     */
    void position (int &_line, int &_column) const;

    /**
     * Retrieve the line on which this cursor is situated.
     * @return line number, where 0 is the first line.
     */
    virtual int line() const;

    /**
     * Set the cursor line to @e line.
     * @note This function is @c virtual to allow reimplementations to do
     *       checks here or to emit signals.
     * @param line new cursor line
     */
    virtual void setLine (int line);

    /**
     * Retrieve the column on which this cursor is situated.
     * @return column number, where 0 is the first column.
     */
    inline int column() const { return m_column; }

    /**
     * Set the cursor column to @e column.
     * @note This function is @c virtual to allow reimplementations to do
     *       checks here or to emit signals.
     * @param column new cursor column
     */
    virtual void setColumn (int column);

    /**
     * Set the current cursor position to @e pos.
     * @note This function is @c virtual to allow reimplementations to do
     *       checks here or to emit signals.
     * @param pos new cursor position
     */
    virtual void setPosition (const Cursor& pos);

    /**
     * Set the cursor position to @e line and @e column.
     * @param line new cursor line
     * @param column new cursor column
     */
    inline void setPosition(int line, int column) { setPosition(Cursor(line, column)); }

    /**
     * Returns the range that this cursor belongs to, if any.
     */
    inline Range* range() const { return m_range; }

    /**
     * @returns true if the cursor is situated at the start of the line, false if it isn't.
     */
    inline bool atStartOfLine() const { return m_column == 0; }
    inline bool atStartOfDocument() const { return m_line == 0 && m_column == 0; }

    inline Cursor& operator=(const Cursor& c) { setPosition(c); return *this; }
    inline friend Cursor operator+(const Cursor& c1, const Cursor& c2) { return Cursor(c1.line() + c2.line(), c1.column() + c2.column()); }
    inline friend Cursor& operator+=(Cursor& c1, const Cursor& c2) { c1.setPosition(c1.line() + c2.line(), c1.column() + c2.column()); return c1; }

    inline friend Cursor operator-(const Cursor& c1, const Cursor& c2) { return Cursor(c1.line() - c2.line(), c1.column() - c2.column()); }
    inline friend Cursor& operator-=(Cursor& c1, const Cursor& c2) { c1.setPosition(c1.line() - c2.line(), c1.column() - c2.column()); return c1; }

    /**
     * == operator
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's and c2's line and column are @e equal.
     */
    inline friend bool operator==(const Cursor& c1, const Cursor& c2)
      { return c1.line() == c2.line() && c1.column() == c2.column(); }

    /**
     * != operator
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's and c2's line and column are @e not equal.
     */
    inline friend bool operator!=(const Cursor& c1, const Cursor& c2)
      { return !(c1 == c2); }

    /**
     * Greater than operator.
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's position is greater than c2's position,
     *         otherwise @e false.
     */
    inline friend bool operator>(const Cursor& c1, const Cursor& c2)
      { return c1.line() > c2.line() || (c1.line() == c2.line() && c1.m_column > c2.m_column); }

    /**
     * >= operator
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's position is greater than or equal to c2's
     *         position, otherwise @e false.
     */
    inline friend bool operator>=(const Cursor& c1, const Cursor& c2)
      { return c1.line() > c2.line() || (c1.line() == c2.line() && c1.m_column >= c2.m_column); }

    /**
     * Less than operator
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's position is greater than or equal to c2's
     *         position, otherwise @e false.
     */
    inline friend bool operator<(const Cursor& c1, const Cursor& c2)
      { return !(c1 >= c2); }

    /**
     * <= operator
     * @param c1 first cursor to compare
     * @param c2 second cursor to compare
     * @return @e true, if c1's position is lesser than or equal to c2's
     *         position, otherwise @e false.
     */
    inline friend bool operator<=(const Cursor& c1, const Cursor& c2)
      { return !(c1 > c2); }

    inline friend kdbgstream& operator<< (kdbgstream& s, const Cursor& cursor) {
      if (&cursor)
        s << "(" << cursor.line() << ", " << cursor.column() << ")";
      else
        s << "(null cursor)";
      return s;
    }

    inline friend kndbgstream& operator<< (kndbgstream& s, const Cursor&) { return s; }

  protected:
    /**
     * Sets the range that this cursor belongs to.
     */
    void setRange(Range* range);

    /**
     * cursor line
     */
    int m_line;

    /**
     * cursor column
     */
    int m_column;

    /**
     * range which owns this cursor, if any
     */
    Range* m_range;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
