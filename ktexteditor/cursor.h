/* This file is part of the KDE project
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

#ifndef __ktexteditor_cursor_h__
#define __ktexteditor_cursor_h__

#include <kdelibs_export.h>

namespace KTextEditor
{

/**
 * The class @p Cursor represents a cursor in the text. Its position is
 * defined by a @e line and a @e column. Lines and columns start with 0,0.
 */
class KTEXTEDITOR_EXPORT Cursor
{
  public:
    /**
     * The default constructor creates a cursor at position (0,0).
     */
    Cursor() : m_line(0), m_column(0) {}

    /**
     * This constructor creates a cursor initialized with @e line
     * and @e column.
     * @param line line for cursor
     * @param column column for cursor
     */
    Cursor(int line, int column) : m_line(line), m_column(column) {}

    /**
     * Virtual destructor
     */
    virtual ~Cursor () {}

    /**
     * Get both the line and column of the cursor position.
     * @param _line will be filled with current cursor line
     * @param _column will be filled with current cursor column
     */
    inline void position (int &_line, int &_column) const { _line = line(); _column = column(); }

    /**
     * Get the cursor line.
     * @return cursor line
     */
    inline int line() const { return m_line; }

    /**
     * Get the cursor column.
     * @return cursor column
     */
    inline int column() const { return m_column; }

    /**
     * Set the current cursor position to @e pos.
     * @note This function is @c virtual to allow reimplementations to do
     *       checks here or to emit signals.
     * @param pos new cursor position
     */
    virtual void setPosition (const Cursor& pos) { m_line = pos.line(); m_column = pos.column(); }

    /**
     * Set the cursor position to @e line and @e column.
     * @note This function is @c virtual to allow reimplementations to do
     *       checks here or to emit signals.
     * @param line new cursor line
     * @param column new cursor column
     */
    virtual void setPosition (int line, int column) { m_line = line; m_column = column; }

    /**
     * Set the cursor line to @e line.
     * @note This function is @c virtual to allow reimplementations to do
     *       checks here or to emit signals.
     * @param line new cursor line
     */
    virtual void setLine (int line) { m_line = line; }

    /**
     * Set the cursor column to @e column.
     * @note This function is @c virtual to allow reimplementations to do
     *       checks here or to emit signals.
     * @param column new cursor column
     */
    virtual void setColumn (int column) { m_column = column; }

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
     * > operator
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
     * lt operator
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

  protected:
    /**
     * cursor line
     */
    int m_line;

    /**
     * cursor column
     */
    int m_column;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
