/* This file is part of the KDE project
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2001-2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_CURSOR_H
#define KDELIBS_KTEXTEDITOR_CURSOR_H

#include <ktexteditor/ktexteditor_export.h>
#include <QDebug>

namespace KTextEditor
{
class Document;
class Range;

/**
 * \short An object which represents a position in a Document.
 *
 * A Cursor is a basic class which contains the line() and column() a position
 * in a Document. It is very lightweight and maintains no affiliation with a
 * particular Document.
 *
 * If you want additional functionality such as the ability to maintain position
 * in a document, see MovingCursor.
 *
 * \note Lines and columns start at 0.
 *
 * \note The Cursor class is designed to be passed via value.
 *
 * \note Think of cursors as having their position at the start of a character,
 *       not in the middle of one.
 *
 * \sa MovingCursor, Range
 */
class KTEXTEDITOR_EXPORT Cursor
{
  public:
    /**
     * The default constructor creates a cursor at position (0,0).
     */
    Cursor()
      : m_line (0)
      , m_column (0)
    {
    }

    /**
     * This constructor creates a cursor initialized with \p line
     * and \p column.
     * \param line line for cursor
     * \param column column for cursor
     */
    Cursor(int line, int column)
      : m_line (line)
      , m_column (column)
    {
    }

    /**
     * Returns whether the current position of this cursor is a valid position
     * (line + column must both be >= 0).
     */
    bool isValid() const
    {
      return m_line >= 0 && m_column >= 0;
    }

    /**
     * Returns an invalid cursor.
     * The returned cursor position is set to (-1, -1).
     * \see isValid()
     */
    static Cursor invalid()
    {
      return Cursor (-1, -1);
    }

    /**
     * Returns a cursor representing the start of any document - i.e., line 0, column 0.
     */
    static Cursor start()
    {
      return Cursor ();
    }

    /**
     * \name Position
     *
     * The following functions provide access to, and manipulation of, the cursor's position.
     * \{
     */
    /**
     * Set the current cursor position to \e position.
     *
     * \param position new cursor position
     */
    void setPosition(const Cursor& position)
    {
      m_line = position.m_line;
      m_column = position.m_column;
    }

    /**
     * \overload
     *
     * Set the cursor position to \e line and \e column.
     *
     * \param line new cursor line
     * \param column new cursor column
     */
    void setPosition(int line, int column)
    {
      m_line = line;
      m_column = column;
    }

    /**
     * Retrieve the line on which this cursor is situated.
     * \return line number, where 0 is the first line.
     */
    int line() const
    {
      return m_line;
    }

    /**
     * Set the cursor line to \e line.
     * \param line new cursor line
     */
    void setLine(int line)
    {
      m_line = line;
    }

    /**
     * Retrieve the column on which this cursor is situated.
     * \return column number, where 0 is the first column.
     */
    int column() const
    {
      return m_column;
    }

    /**
     * Set the cursor column to \e column.
     * \param column new cursor column
     */
    void setColumn(int column)
    {
      m_column = column;
    }

    /**
     * Determine if this cursor is located at the start of a line.
     * \return \e true if the cursor is situated at the start of the line, \e false if it isn't.
     */
    bool atStartOfLine() const
    {
      return m_column == 0;
    }

    /**
     * Determine if this cursor is located at the start of a document.
     * \return \e true if the cursor is situated at the start of the document, \e false if it isn't.
     */
    bool atStartOfDocument() const
    {
      return m_line == 0 && m_column == 0;
    }

    /**
     * Get both the line and column of the cursor position.
     * \param line will be filled with current cursor line
     * \param column will be filled with current cursor column
     */
    void position (int &line, int &column) const
    {
      line = m_line;
      column = m_column;
    }
    //!\}

    /**
     * Addition operator. Takes two cursors and returns their summation.
     * \param c1 the first position
     * \param c2 the second position
     * \return a the summation of the two input cursors
     */
    inline friend Cursor operator+(const Cursor& c1, const Cursor& c2)
      { return Cursor(c1.line() + c2.line(), c1.column() + c2.column()); }

    /**
     * Addition assignment operator. Adds \p c2 to this cursor.
     * \param c1 the cursor being added to
     * \param c2 the position to add
     * \return a reference to the cursor which has just been added to
     */
    inline friend Cursor& operator+=(Cursor& c1, const Cursor& c2)
      { c1.setPosition(c1.line() + c2.line(), c1.column() + c2.column()); return c1; }

    /**
     * Subtraction operator. Takes two cursors and returns the subtraction
     * of \p c2 from \p c1.
     *
     * \param c1 the first position
     * \param c2 the second position
     * \return a cursor representing the subtraction of \p c2 from \p c1
     */
    inline friend Cursor operator-(const Cursor& c1, const Cursor& c2)
      { return Cursor(c1.line() - c2.line(), c1.column() - c2.column()); }

    /**
     * Subtraction assignment operator. Subtracts \p c2 from \p c1.
     * \param c1 the cursor being subtracted from
     * \param c2 the position to subtract
     * \return a reference to the cursor which has just been subtracted from
     */
    inline friend Cursor& operator-=(Cursor& c1, const Cursor& c2)
      { c1.setPosition(c1.line() - c2.line(), c1.column() - c2.column()); return c1; }

    /**
     * Equality operator.
     *
     * \note comparison between two invalid cursors is undefined.
     *       comparison between and invalid and a valid cursor will always be \e false.
     *
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's and c2's line and column are \e equal.
     */
    inline friend bool operator==(const Cursor& c1, const Cursor& c2)
      { return c1.line() == c2.line() && c1.column() == c2.column(); }

    /**
     * Inequality operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's and c2's line and column are \e not equal.
     */
    inline friend bool operator!=(const Cursor& c1, const Cursor& c2)
      { return !(c1 == c2); }

    /**
     * Greater than operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is greater than c2's position,
     *         otherwise \e false.
     */
    inline friend bool operator>(const Cursor& c1, const Cursor& c2)
      { return c1.line() > c2.line() || (c1.line() == c2.line() && c1.m_column > c2.m_column); }

    /**
     * Greater than or equal to operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is greater than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator>=(const Cursor& c1, const Cursor& c2)
      { return c1.line() > c2.line() || (c1.line() == c2.line() && c1.m_column >= c2.m_column); }

    /**
     * Less than operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is greater than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator<(const Cursor& c1, const Cursor& c2)
      { return !(c1 >= c2); }

    /**
     * Less than or equal to operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is lesser than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator<=(const Cursor& c1, const Cursor& c2)
      { return !(c1 > c2); }

    /**
     * qDebug() stream operator.  Writes this cursor to the debug output in a nicely formatted way.
     */
    inline friend QDebug operator<< (QDebug s, const Cursor& cursor) {
      if (&cursor)
        s.nospace() << "(" << cursor.line() << ", " << cursor.column() << ")";
      else
        s.nospace() << "(null cursor)";
      return s.space();
    }

   private:
    /**
     * \internal
     *
     * Cursor line
     */
    int m_line;

    /**
     * \internal
     *
     * Cursor column
     */
    int m_column;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
