/* This file is part of the KDE project
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  Based on code of the SmartCursor/Range by:
 *  Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
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

#ifndef KDELIBS_KTEXTEDITOR_MOVINGCURSOR_H
#define KDELIBS_KTEXTEDITOR_MOVINGCURSOR_H

#include <kdebug.h>
#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/cursor.h>
#include <ktexteditor/document.h>

namespace KTextEditor
{

class MovingRange;

/**
 * \short A Cursor which is bound to a specific Document, and maintains its position.
 *
 * \ingroup kte_group_moving_classes
 *
 * A MovingCursor is an extension of the basic Cursor class. It maintains its
 * position in the document. As a result of this, MovingCursor%s may not be copied, as they need
 * to maintain a connection to the associated Document.
 *
 * Create a new MovingCursor like this:
 * \code
 * // Retrieve the MovingInterface
 * KTextEditor::MovingInterface* moving =
 *     qobject_cast<KTextEditor::MovingInterface*>( yourDocument );
 *
 * if ( moving ) {
 *     KTextEditor::MovingCursor* cursor = moving->newMovingCursor();
 * }
 * \endcode
 *
 * When finished with a MovingCursor, simply delete it.
 * If the document the cursor belong to is deleted, it will get deleted automatically.
 *
 * \sa Cursor, Range, MovingRange and MovingInterface.
 *
 * \author Christoph Cullmann \<cullmann@kde.org\>
 *
 * \since 4.5
 */
class KTEXTEDITOR_EXPORT MovingCursor
{
  //
  // sub types
  //
  public:
    /**
     * Insert behavior of this cursor, should it stay if text is insert at it's position
     * or should it move.
     */
    enum InsertBehavior {
      StayOnInsert = 0x0, ///< stay on insert
      MoveOnInsert = 0x1  ///< move on insert
    };

    /**
     * Wrap behavior for end of line treatement used in move().
     */
    enum WrapBehavior {
      Wrap = 0x0,  ///< wrap at end of line
      NoWrap = 0x1 ///< do not wrap at end of line
    };

  //
  // stuff that needs to be implemented by editor part cusors
  //
  public:
    /**
     * Set insert behavior.
     * @param insertBehavior new insert behavior
     */
    virtual void setInsertBehavior (InsertBehavior insertBehavior) = 0;

    /**
     * Get current insert behavior.
     * @return current insert behavior
     */
    virtual InsertBehavior insertBehavior () const = 0;

    /**
     * Gets the document to which this cursor is bound.
     * \return a pointer to the document
     */
    virtual Document *document () const = 0;

    /**
     * Get range this cursor belongs to, if any
     * @return range this pointer is part of, else 0
     */
    virtual MovingRange *range () const = 0;

    /**
     * Set the current cursor position to \e position.
     *
     * \param position new cursor position
     */
    virtual void setPosition (const KTextEditor::Cursor& position) = 0;

    /**
     * Retrieve the line on which this cursor is situated.
     * \return line number, where 0 is the first line.
     */
    virtual int line() const = 0;

    /**
     * Retrieve the column on which this cursor is situated.
     * \return column number, where 0 is the first column.
     */
    virtual int column() const = 0;

    /**
     * Destruct the moving cursor.
     */
    virtual ~MovingCursor ();

  //
  // forbidden stuff
  //
  protected:
    /**
     * For inherited class only.
     */
    MovingCursor ();

  private:
    /**
     * no copy constructor, don't allow this to be copied.
     */
    MovingCursor (const MovingCursor &);

    /**
     * no assignment operator, no copying around clever cursors.
     */
    MovingCursor &operator= (const MovingCursor &);

  //
  // convenience API
  //
  public:

    /**
     * Returns whether the current position of this cursor is a valid position,
     * i.e. whether line() >= 0 and column() >= 0.
     * \return \e true , if the cursor position is valid, otherwise \e false
     */
    inline bool isValid() const {
      return line() >= 0 && column() >= 0;
    }

    /**
     * Check whether the current position of this cursor is a valid text
     * position.
     * \return \e true , if the cursor is a valid text position , otherwise \e false
     */
    inline bool isValidTextPosition() const {
      return isValid() && line() < document()->lines() && column() <= document()->lineLength(line());
    }

    /**
     * \overload
     *
     * Set the cursor position to \e line and \e column.
     *
     * \param line new cursor line
     * \param column new cursor column
     */
    void setPosition (int line, int column);

    /**
     * Set the cursor line to \e line.
     * \param line new cursor line
     */
    void setLine(int line);

    /**
     * Set the cursor column to \e column.
     * \param column new cursor column
     */
    void setColumn(int column);

    /**
     * Determine if this cursor is located at column 0 of a valid text line.
     *
     * \return \e true if cursor is a valid text position and column()=0, otherwise \e false.
     */
    bool atStartOfLine() const;

    /**
     * Determine if this cursor is located at the end of the current line.
     *
     * \return \e true if the cursor is situated at the end of the line, otherwise \e false.
     */
    bool atEndOfLine() const;

    /**
     * Determine if this cursor is located at line 0 and column 0.
     *
     * \return \e true if the cursor is at start of the document, otherwise \e false.
     */
    bool atStartOfDocument() const;

    /**
     * Determine if this cursor is located at the end of the last line in the
     * document.
     *
     * \return \e true if the cursor is at the end of the document, otherwise \e false.
     */
    bool atEndOfDocument() const;

    /**
     * Moves the cursor to the next line and sets the column to 0. If the cursor
     * position is already in the last line of the document, the cursor position
     * remains unchanged and the return value is \e false.
     *
     * \return \e true on success, otherwise \e false
     */
    bool gotoNextLine();

    /**
     * Moves the cursor to the previous line and sets the column to 0. If the
     * cursor position is already in line 0, the cursor position remains
     * unchanged and the return value is \e false.
     *
     * \return \e true on success, otherwise \e false
     */
    bool gotoPreviousLine();

    /**
     * Moves the cursor \p chars character forward or backwards. If \e wrapBehavior
     * equals WrapBehavior::Wrap, the cursor is automatically wrapped to the
     * next line at the end of a line.
     *
     * When moving backwards, the WrapBehavior does not have any effect.
     * \note If the cursor could not be moved the amount of chars requested,
     *       the cursor is not moved at all!
     *
     * \return \e true on success, otherwise \e false
     */
    bool move(int chars, WrapBehavior wrapBehavior = Wrap);

    /**
     * Convert this clever cursor into a dumb one.
     * Even if this cursor belongs to a range, the created one not.
     * @return normal cursor
     */
    const Cursor toCursor () const { return Cursor (line(), column()); }

    /**
     * Convert this clever cursor into a dumb one. Equal to toCursor, allowing to use implicit conversion.
     * Even if this cursor belongs to a range, the created one not.
     * @return normal cursor
     */
    operator const Cursor () const { return Cursor (line(), column()); }

//
// operators for: MovingCursor <-> MovingCursor
//
    /**
     * Equality operator.
     *
     * \note comparison between two invalid cursors is undefined.
     *       comparison between an invalid and a valid cursor will always be \e false.
     *
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's and c2's line and column are \e equal.
     */
    inline friend bool operator==(const MovingCursor& c1, const MovingCursor& c2)
      { return c1.line() == c2.line() && c1.column() == c2.column(); }

    /**
     * Inequality operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's and c2's line and column are \e not equal.
     */
    inline friend bool operator!=(const MovingCursor& c1, const MovingCursor& c2)
      { return !(c1 == c2); }

    /**
     * Greater than operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is greater than c2's position,
     *         otherwise \e false.
     */
    inline friend bool operator>(const MovingCursor& c1, const MovingCursor& c2)
      { return c1.line() > c2.line() || (c1.line() == c2.line() && c1.column() > c2.column()); }

    /**
     * Greater than or equal to operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is greater than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator>=(const MovingCursor& c1, const MovingCursor& c2)
      { return c1.line() > c2.line() || (c1.line() == c2.line() && c1.column() >= c2.column()); }

    /**
     * Less than operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is greater than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator<(const MovingCursor& c1, const MovingCursor& c2)
      { return !(c1 >= c2); }

    /**
     * Less than or equal to operator.
     * \param c1 first cursor to compare
     * \param c2 second cursor to compare
     * \return \e true, if c1's position is lesser than or equal to c2's
     *         position, otherwise \e false.
     */
    inline friend bool operator<=(const MovingCursor& c1, const MovingCursor& c2)
      { return !(c1 > c2); }

    /**
     * kDebug() stream operator. Writes this cursor to the debug output in a nicely formatted way.
     * @param s debug stream
     * @param cursor cursor to print
     * @return debug stream
     */
    inline friend QDebug operator<< (QDebug s, const MovingCursor *cursor) {
      if (cursor)
        s.nospace() << "(" << cursor->line() << ", " << cursor->column() << ")";
      else
        s.nospace() << "(null cursor)";
      return s.space();
    }

    /**
     * kDebug() stream operator. Writes this cursor to the debug output in a nicely formatted way.
     * @param s debug stream
     * @param cursor cursor to print
     * @return debug stream
     */
    inline friend QDebug operator<< (QDebug s, const MovingCursor &cursor) {
      return s << &cursor;
    }
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
