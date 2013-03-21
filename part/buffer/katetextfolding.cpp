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
  
TextFolding::TextFolding (const TextBuffer &buffer)
  : QObject ()
  , m_buffer (buffer)
{
}

bool TextFolding::newFoldingRange (const KTextEditor::Range &range, FoldingRangeState state)
{
  // use qLowerBound + qUpperBound to find all ranges we overlap toplevel
  // if we overlap none => insert toplevel
  // if we contain one or multiple => either try to move the inside the new one or if that fails abort
  // if we are containted in one => descend and do this again for the new vector!
  
  return false;
}
  
}
