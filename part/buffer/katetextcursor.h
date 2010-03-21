/*  This file is part of the Kate project.
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

#ifndef KATE_TEXTCURSOR_H
#define KATE_TEXTCURSOR_H

#include <ktexteditor/cursor.h>

namespace Kate {

class TextBuffer;
class TextBlock;
class TextRange;

/**
 * Class representing a 'clever' text cursor.
 * It will automagically move if the text inside the buffer it belongs to is modified.
 * By intention no subclass of KTextEditor::Cursor, must be converted manually.
 */
class TextCursor {
  // range wants direct access to some internals
  friend class TextRange;

  // this is a friend, because this is needed to efficiently transfer cursors from on to an other block
  friend class TextBlock;

  public:
    /**
     * Insert behavior of this cursor, should it stay if text is insert at it's position
     * or should it move.
     */
    enum InsertBehavior {
      StayOnInsert = 0x0,
      MoveOnInsert = 0x1
    };

    /**
     * Construct a text cursor.
     * @param buffer text buffer this cursor belongs to
     * @param position wanted cursor position, if not valid for given buffer, will lead to invalid cursor
     * @param insertBehavior behavior of this cursor on insert of text at it's position
     */
    TextCursor (TextBuffer &buffer, const KTextEditor::Cursor &position, InsertBehavior insertBehavior);

  private:
    /**
     * Construct a text cursor with given range as parent, private, used by TextRange constructor only.
     * @param buffer text buffer this cursor belongs to
     * @param range text range this cursor is part of
     * @param position wanted cursor position, if not valid for given buffer, will lead to invalid cursor
     * @param insertBehavior behavior of this cursor on insert of text at it's position
     */
    TextCursor (TextBuffer &buffer, TextRange *range, const KTextEditor::Cursor &position, InsertBehavior insertBehavior);

  public:
    /**
     * Destruct the text cursor
     */
    virtual ~TextCursor ();

  private:
    /**
     * no copy constructor, don't allow this to be copied
     */
    TextCursor (const TextCursor &);

    /**
     * no assignment operator, no copying around clever cursors
     */
    TextCursor &operator= (const TextCursor &);

  public:
    /**
     * Set the current cursor position to \e position.
     *
     * \param position new cursor position
     */
    void setPosition (const KTextEditor::Cursor& position);

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
     * Retrieve the line on which this cursor is situated.
     * \return line number, where 0 is the first line.
     */
    int line() const;

    /**
     * Set the cursor line to \e line.
     * \param line new cursor line
     */
    void setLine(int line);

    /**
     * Retrieve the column on which this cursor is situated.
     * \return column number, where 0 is the first column.
     */
    int column() const { return m_column; }

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
    const KTextEditor::Cursor toCursor () const { return KTextEditor::Cursor (line(), column()); }

    /**
     * Convert this clever cursor into a dumb one. Equal to toCursor, allowing to use implicit conversion.
     * Even if this cursor belongs to a range, the created one not.
     * @return normal cursor
     */
    operator const KTextEditor::Cursor () const { return KTextEditor::Cursor (line(), column()); }

    /**
     * Get range this cursor belongs to, if any
     * @return range this pointer is part of, else 0
     */
    TextRange *range () { return m_range; }

    /**
     * Get block this cursor belongs to, if any
     * @return block this pointer is part of, else 0
     */
    TextBlock *block () { return m_block; }

    /**
     * Get offset into block this cursor belongs to, if any
     * @return offset into block this pointer is part of, else -1
     */
    int lineInBlock () const { if (m_block) return m_line; return -1; }

  private:
    /**
     * Set the current cursor position to \e position.
     * Internal helper to allow the same code be used for constructor and
     * setPosition.
     *
     * @param position new cursor position
     * @param init is this the initial setup of the position in the constructor?
     */
    void setPosition (const KTextEditor::Cursor& position, bool init);

  private:
    /**
     * parent text buffer
     * is a reference, and no pointer, as this must always exist and can't change
     */
    TextBuffer &m_buffer;

    /**
     * range this cursor belongs to
     * may be null, then no range owns this cursor
     * can not change after initial assignment
     */
    TextRange * const m_range;

    /**
     * parent text block, valid cursors always belong to a block, else they are invalid.
     */
    TextBlock *m_block;

    /**
     * line, offset in block, or -1
     */
    int m_line;

    /**
     * column
     */
    int m_column;

    /**
     * should this cursor move on insert
     */
    bool m_moveOnInsert;
};

}

#endif
