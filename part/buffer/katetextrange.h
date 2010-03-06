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

#ifndef KATE_TEXTRANGE_H
#define KATE_TEXTRANGE_H

#include <ktexteditor/range.h>

#include "katetextcursor.h"

namespace Kate {

class TextBuffer;

/**
 * Class representing a 'clever' text range.
 * It will automagically move if the text inside the buffer it belongs to is modified.
 * By intention no subclass of KTextEditor::Range, must be converted manually.
 * A TextRange is not allowed to be empty, as soon as start == end position, it will become
 * automatically invalid!
 */
class TextRange {
  // this is a friend, block changes might invalidate ranges...
  friend class TextBlock;

  public:
     /// Determine how the range reacts to characters inserted immediately outside the range.
    enum InsertBehavior {
      /// Don't expand to encapsulate new characters in either direction. This is the default.
      DoNotExpand = 0x0,
      /// Expand to encapsulate new characters to the left of the range.
      ExpandLeft = 0x1,
      /// Expand to encapsulate new characters to the right of the range.
      ExpandRight = 0x2
    };
    Q_DECLARE_FLAGS(InsertBehaviors, InsertBehavior)

    /**
     * Construct a text range.
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     * @param buffer parent text buffer
     * @param range The initial text range assumed by the new range.
     * @param insertBehavior Define whether the range should expand when text is inserted adjacent to the range.
     */
    TextRange (TextBuffer &buffer, const KTextEditor::Range &range, InsertBehaviors insertBehavior);

    /**
     * Destruct the text block
     */
    virtual ~TextRange ();

  private:
    /**
     * no copy constructor, don't allow this to be copied
     */
    TextRange (const TextRange &);

    /**
     * no assignment operator, no copying around clever range
     */
    TextRange &operator= (const TextRange &);

  public:
    /**
     * Set the range of this range.
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     * @param range new range for this clever range
     */
    void setRange (const KTextEditor::Range &range);

    /**
     * \overload
     * Set the range of this range
     * A TextRange is not allowed to be empty, as soon as start == end position, it will become
     * automatically invalid!
     * @param start new start for this clever range
     * @param end new end for this clever range
     */
    void setRange (const KTextEditor::Cursor &start, const KTextEditor::Cursor &end);

    /**
     * Retrieve start cursor of this range, read-only.
     * @return start cursor
     */
    const TextCursor &start () const { return m_start; }

    /**
     * Retrieve end cursor of this range, read-only.
     * @return end cursor
     */
    const TextCursor &end () const { return m_end; }

    /**
     * Convert this clever range into a dumb one.
     * @return normal range
     */
    KTextEditor::Range toRange () const { return KTextEditor::Range (start().toCursor(), end().toCursor()); }

  private:
    /**
     * Check if range is valid, used by constructor and setRange.
     * If at least one cursor is invalid, both will set to invalid.
     * Same if range itself is invalid (start >= end).
     */
    void checkValidity ();

  private:
    /**
     * parent text buffer
     * is a reference, and no pointer, as this must always exist and can't change
     */
    TextBuffer &m_buffer;

    /**
     * Start cursor for this range, is a clever cursor
     */
    TextCursor m_start;

    /**
     * End cursor for this range, is a clever cursor
     */
    TextCursor m_end;
};

}

#endif
