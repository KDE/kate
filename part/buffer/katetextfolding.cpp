/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2013 Christoph Cullmann <cullmann@kde.org>
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

#include "katetextfolding.h"
#include "katetextbuffer.h"
#include "katetextrange.h"

namespace Kate {
  
TextFolding::FoldingRange::FoldingRange (TextBuffer &buffer, const KTextEditor::Range &range, FoldingRangeState _state)
  : start (new TextCursor (buffer, range.start(), KTextEditor::MovingCursor::MoveOnInsert))
  , end (new TextCursor (buffer, range.end(), KTextEditor::MovingCursor::MoveOnInsert))
  , parent (0)
  , state (_state)
{
}
  
TextFolding::FoldingRange::~FoldingRange ()
{
  /**
   * kill all our data!
   * this will recurse all sub-structures!
   */
  delete start;
  delete end;
  qDeleteAll (nestedRanges);
}
  
TextFolding::TextFolding (TextBuffer &buffer)
  : QObject ()
  , m_buffer (buffer)
{
}

bool TextFolding::newFoldingRange (const KTextEditor::Range &range, FoldingRangeState state)
{
  /**
   * sort out invalid and empty ranges
   * that makes no sense, they will never grow again!
   */
  if (!range.isValid() || range.isEmpty())
    return false;
  
  /**
   * create new folding region that we want to insert
   * this will internally create moving cursors!
   */
  FoldingRange *newRange = new FoldingRange (m_buffer, range, state);
  
  /**
   * the construction of the text cursors might have invalidated this
   * check and bail out if that happens
   * bail out, too, if it can't be inserted!
   */
  if (    !newRange->start->isValid()
       || !newRange->end->isValid()
       || !insertNewFoldingRange (m_foldingRanges, newRange)) {
    /**
     * cleanup and be done
     */
    delete newRange;
    return false;
  }
  
  /**
   * all went fine, newRange is now registered internally!
   */
  return true;
}

bool TextFolding::insertNewFoldingRange (FoldingRange::Vector &existingRanges, FoldingRange *newRange)
{
  // use qLowerBound + qUpperBound to find all ranges we overlap toplevel
  // if we overlap none => insert toplevel
  // if we contain one or multiple => either try to move the inside the new one or if that fails abort
  // if we are containted in one => descend and do this again for the new vector!
  
  return false;
}

}
