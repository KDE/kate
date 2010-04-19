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

#include "katetextcursor.h"
#include "katetextbuffer.h"

namespace Kate {

TextCursor::TextCursor (TextBuffer &buffer, const KTextEditor::Cursor &position, InsertBehavior insertBehavior)
  : m_buffer (buffer)
  , m_range (0)
  , m_block (0)
  , m_line (-1)
  , m_column (-1)
  , m_moveOnInsert (insertBehavior == MoveOnInsert)
{
  // init position
  setPosition (position, true);
}

TextCursor::TextCursor (TextBuffer &buffer, TextRange *range, const KTextEditor::Cursor &position, InsertBehavior insertBehavior)
  : m_buffer (buffer)
  , m_range (range)
  , m_block (0)
  , m_line (-1)
  , m_column (-1)
  , m_moveOnInsert (insertBehavior == MoveOnInsert)
{
  // init position
  setPosition (position, true);
}

TextCursor::~TextCursor ()
{
  // remove cursor from block or buffer
  if (m_block)
    m_block->m_cursors.remove (this);

  // only cursors without range are here!
  else if (!m_range)
    m_buffer.m_invalidCursors.remove (this);
}

void TextCursor::setPosition(const KTextEditor::Cursor& position, bool init)
{
  // any change or init? else do nothing
  if (!init && position.line() == line() && position.column() == m_column)
    return;

  // remove cursor from old block in any case
  if (m_block)
    m_block->m_cursors.remove (this);

  // first: validate the line and column, else invalid
  if (position.column() < 0 || position.line () < 0 || position.line () >= m_buffer.lines ()) {
    if (!m_range)
      m_buffer.m_invalidCursors.insert (this);
    m_block = 0;
    m_line = m_column = -1;
    return;
  }

  // else, find block
  TextBlock *block = m_buffer.blockForIndex (m_buffer.blockForLine (position.line()));

  // get line
  TextLine textLine = block->line (position.line());

#if 0 // this is no good idea, smart cursors don't do that, too, for non-wrapping cursors
  // now, validate column, else stay invalid
  if (position.column() > textLine->text().size()) {
    if (!m_range)
      m_buffer.m_invalidCursors.insert (this);
    m_block = 0;
    m_line = m_column = -1;
    return;
  }
#endif

  // else: valid cursor
  m_block = block;
  m_line = position.line () - m_block->startLine ();
  m_column = position.column ();
  m_block->m_cursors.insert (this);
}

void TextCursor::setPosition(const KTextEditor::Cursor& position)
{
    setPosition(position, false);
}

int TextCursor::line() const
{
  // invalid cursor have no block
  if (!m_block)
    return -1;

  // else, calculate real line
  return m_block->startLine () + m_line;
}

KTextEditor::Document *Kate::TextCursor::document () const
{
  return m_buffer.document();
}

KTextEditor::MovingRange *Kate::TextCursor::range () const
{
  return m_range;
}

}
