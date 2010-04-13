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

#include "katetextrange.h"
#include "katetextbuffer.h"

namespace Kate {

TextRange::TextRange (TextBuffer &buffer, const KTextEditor::Range &range, InsertBehaviors insertBehavior)
  : m_buffer (buffer)
  , m_start (buffer, this, range.start(), (insertBehavior & ExpandLeft) ? Kate::TextCursor::StayOnInsert : Kate::TextCursor::MoveOnInsert)
  , m_end (buffer, this, range.end(), (insertBehavior & ExpandRight) ? Kate::TextCursor::MoveOnInsert : Kate::TextCursor::StayOnInsert)
{
  // remember this range in buffer
  m_buffer.m_ranges.insert (this);

  // check if range now invalid
  checkValidity ();
}

TextRange::~TextRange ()
{
  // remove range from m_ranges
  fixLookup (m_start.line(), m_end.line(), -1, -1);

  // remove this range from the buffer
  m_buffer.m_ranges.remove (this);
}

void TextRange::setRange (const KTextEditor::Range &range)
{
  // remember old line range
  int oldStartLine = m_start.line();
  int oldEndLine = m_end.line();

  // change start and end cursor
  m_start.setPosition (range.start ());
  m_end.setPosition (range.end ());

  // check if range now invalid
  checkValidity (oldStartLine, oldEndLine);
}

void TextRange::setRange (const KTextEditor::Cursor &start, const KTextEditor::Cursor &end)
{
  // just use other function, KTextEditor::Range will handle some normalization
  setRange (KTextEditor::Range (start, end));
}

void TextRange::checkValidity (int oldStartLine, int oldEndLine)
{
  // check if any cursor is invalid or the range is zero size
  // if yes, invalidate this range
  if (!m_start.toCursor().isValid() || !m_end.toCursor().isValid() || m_end.toCursor() <= m_start.toCursor()) {
    m_start.setPosition (-1, -1);
    m_end.setPosition (-1, -1);
  }

  // fix lookup
  fixLookup (oldStartLine, oldEndLine, m_start.line(), m_end.line());
}

void TextRange::fixLookup (int oldStartLine, int oldEndLine, int startLine, int endLine)
{
  // nothing changed?
  if (oldStartLine == startLine && oldEndLine == endLine)
    return;

  // now, not both can be invalid
  Q_ASSERT (oldStartLine >= 0 || startLine >= 0);
  Q_ASSERT (oldEndLine >= 0 || endLine >= 0);

  // get full range
  int startLineMin = oldStartLine;
  if (oldStartLine == -1 || (startLine != -1 && startLine < oldStartLine))
    startLineMin = startLine;

  int endLineMax = oldEndLine;
  if (oldEndLine == -1 || endLine > oldEndLine)
    endLineMax = endLine;

  // get start block
  int blockIndex = m_buffer.blockForLine (startLineMin);
  Q_ASSERT (blockIndex >= 0);

  // remove this range from m_ranges
  for (; blockIndex < m_buffer.m_blocks.size(); ++blockIndex) {
    // get block
    TextBlock *block = m_buffer.m_blocks[blockIndex];

    // either insert or remove range
    if ((endLine < block->startLine()) || (startLine >= (block->startLine() + block->lines())))
      block->m_ranges.remove (this);
    else
      block->m_ranges.insert (this);

    // ok, reached end block
    if (endLineMax < (block->startLine() + block->lines()))
      return;
  }

  // we should not be here, really, then endLine is wrong
  Q_ASSERT (false);
}

}
