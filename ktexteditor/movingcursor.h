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

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/cursor.h>

namespace KTextEditor
{

class Document;
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
      StayOnInsert = 0x0,
      MoveOnInsert = 0x1
    };

  //
  // stuff that needs to be implemented by editor part cusors
  //
  public:
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
     * Retrieve the insertion behavior
     * @return insertion behavior of this cursor
     */
    virtual InsertBehavior insertBehavior () const = 0;

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
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
