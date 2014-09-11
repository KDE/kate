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

TextRange::TextRange (TextBuffer &buffer, const KTextEditor::Range &range, InsertBehaviors insertBehavior, EmptyBehavior emptyBehavior)
  : m_buffer (buffer)
  , m_start (buffer, this, range.start(), (insertBehavior & ExpandLeft) ? Kate::TextCursor::StayOnInsert : Kate::TextCursor::MoveOnInsert)
  , m_end (buffer, this, range.end(), (insertBehavior & ExpandRight) ? Kate::TextCursor::MoveOnInsert : Kate::TextCursor::StayOnInsert)
  , m_view (0)
  , m_feedback (0)
  , m_zDepth (0.0)
  , m_attributeOnlyForViews (false)
  , m_invalidateIfEmpty (emptyBehavior == InvalidateIfEmpty)
{
  // remember this range in buffer
  m_buffer.m_ranges.insert (this);

  // check if range now invalid, there can happen no feedback, as m_feedback == 0
  checkValidity ();
}

TextRange::~TextRange ()
{
  /**
   * reset feedback, don't want feedback during destruction
   */
  m_feedback = 0;

  // remove range from m_ranges
  fixLookup (m_start.line(), m_end.line(), -1, -1);

  // remove this range from the buffer
  m_buffer.m_ranges.remove (this);

  /**
   * trigger update, if we have attribute
   * notify right view
   * here we can ignore feedback, even with feedback, we want none if the range is deleted!
   */
  if (m_attribute)
    m_buffer.notifyAboutRangeChange (m_view, m_start.line(), m_end.line(), true /* we have a attribute */);
}

void TextRange::setInsertBehaviors (InsertBehaviors _insertBehaviors)
{
  /**
   * nothing to do?
   */
  if (_insertBehaviors == insertBehaviors ())
    return;

  /**
   * modify cursors
   */
  m_start.setInsertBehavior ((_insertBehaviors & ExpandLeft) ? KTextEditor::MovingCursor::StayOnInsert : KTextEditor::MovingCursor::MoveOnInsert);
  m_end.setInsertBehavior ((_insertBehaviors & ExpandRight) ? KTextEditor::MovingCursor::MoveOnInsert : KTextEditor::MovingCursor::StayOnInsert);

  /**
   * notify world
   */
  if (m_attribute || m_feedback)
    m_buffer.notifyAboutRangeChange (m_view, m_start.line(), m_end.line(), true /* we have a attribute */);
}

KTextEditor::MovingRange::InsertBehaviors TextRange::insertBehaviors () const
{
  InsertBehaviors behaviors = DoNotExpand;

  if (m_start.insertBehavior() == KTextEditor::MovingCursor::StayOnInsert)
    behaviors |= ExpandLeft;

  if (m_end.insertBehavior() == KTextEditor::MovingCursor::MoveOnInsert)
    behaviors |= ExpandRight;

  return behaviors;
}

void TextRange::setEmptyBehavior (EmptyBehavior emptyBehavior)
{
  /**
   * nothing to do?
   */
  if (m_invalidateIfEmpty == (emptyBehavior == InvalidateIfEmpty))
    return;

  /**
   * remember value
   */
  m_invalidateIfEmpty = (emptyBehavior == InvalidateIfEmpty);

  /**
   * invalidate range?
   */
  if (end() <= start())
    setRange (KTextEditor::Range::invalid());
}

void TextRange::setRange (const KTextEditor::Range &range)
{
  // avoid work if nothing changed!
  if (range == toRange())
    return;

  // remember old line range
  int oldStartLine = m_start.line();
  int oldEndLine = m_end.line();

  // change start and end cursor
  m_start.setPosition (range.start ());
  m_end.setPosition (range.end ());

  // check if range now invalid, don't emit feedback here, will be handled below
  // otherwise you can't delete ranges in feedback!
  checkValidity (oldStartLine, oldEndLine, false);

  // no attribute or feedback set, be done
  if (!m_attribute && !m_feedback)
    return;

  // get full range
  int startLineMin = oldStartLine;
  if (oldStartLine == -1 || (m_start.line() != -1 && m_start.line() < oldStartLine))
    startLineMin = m_start.line();

  int endLineMax = oldEndLine;
  if (oldEndLine == -1 || m_end.line() > oldEndLine)
    endLineMax = m_end.line();

  /**
   * notify buffer about attribute change, it will propagate the changes
   * notify right view
   */
  m_buffer.notifyAboutRangeChange (m_view, startLineMin, endLineMax, m_attribute);

  // perhaps need to notify stuff!
  if (m_feedback) {
    // do this last: may delete this range
    if (!toRange().isValid())
      m_feedback->rangeInvalid (this);
    else if (toRange().isEmpty())
      m_feedback->rangeEmpty (this);
  }
}

void TextRange::checkValidity (int oldStartLine, int oldEndLine, bool notifyAboutChange)
{
  /**
   * check if any cursor is invalid or the range is zero size and it should be invalidated then
   */
  if (!m_start.isValid() || !m_end.isValid() || (m_invalidateIfEmpty && m_end <= m_start)) {
    m_start.setPosition (-1, -1);
    m_end.setPosition (-1, -1);
  }

  /**
   * for ranges which are allowed to become empty, normalize them, if the end has moved to the front of the start
   */
  if (!m_invalidateIfEmpty && m_end < m_start)
    m_end.setPosition (m_start);

  // fix lookup
  fixLookup (oldStartLine, oldEndLine, m_start.line(), m_end.line());

  // perhaps need to notify stuff!
  if (notifyAboutChange && m_feedback) {
    m_buffer.notifyAboutRangeChange (m_view, m_start.line(), m_end.line(), false /* attribute not interesting here */);

    // do this last: may delete this range
    if (!toRange().isValid())
      m_feedback->rangeInvalid (this);
    else if (toRange().isEmpty())
      m_feedback->rangeEmpty (this);
  }
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
      block->removeRange (this);
    else
      block->updateRange (this);

    // ok, reached end block
    if (endLineMax < (block->startLine() + block->lines()))
      return;
  }

  // we should not be here, really, then endLine is wrong
  Q_ASSERT (false);
}

void TextRange::setView (KTextEditor::View *view)
{
  /**
   * nothing changes, nop
   */
  if (view == m_view)
    return;

  /**
   * remember the new attribute
   */
  m_view = view;

  /**
   * notify buffer about attribute change, it will propagate the changes
   * notify all views (can be optimized later)
   */
  if (m_attribute || m_feedback)
    m_buffer.notifyAboutRangeChange (0, m_start.line(), m_end.line(), m_attribute);
}

void TextRange::setAttribute ( KTextEditor::Attribute::Ptr attribute )
{
  /**
   * remember the new attribute
   */
  m_attribute = attribute;

  /**
   * notify buffer about attribute change, it will propagate the changes
   * notify right view
   */
  m_buffer.notifyAboutRangeChange (m_view, m_start.line(), m_end.line(), m_attribute);
}

void TextRange::setFeedback (KTextEditor::MovingRangeFeedback *feedback)
{
  /**
   * nothing changes, nop
   */
  if (feedback == m_feedback)
    return;

  /**
   * remember the new feedback object
   */
  m_feedback = feedback;

  /**
   * notify buffer about feedback change, it will propagate the changes
   * notify right view
   */
  m_buffer.notifyAboutRangeChange (m_view, m_start.line(), m_end.line(), m_attribute);
}

void TextRange::setAttributeOnlyForViews (bool onlyForViews)
{
    /**
     * just set the value, no need to trigger updates, printing is not interruptable
     */
    m_attributeOnlyForViews = onlyForViews;
}

void TextRange::setZDepth (qreal zDepth)
{
  /**
   * nothing changes, nop
   */
  if (zDepth == m_zDepth)
    return;

  /**
   * remember the new attribute
   */
  m_zDepth = zDepth;

  /**
   * notify buffer about attribute change, it will propagate the changes
   */
  if (m_attribute)
    m_buffer.notifyAboutRangeChange (m_view, m_start.line(), m_end.line(), m_attribute);
}

KTextEditor::Document *Kate::TextRange::document () const
{
    return m_buffer.document();
}

}
