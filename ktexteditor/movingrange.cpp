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

#include "movingrange.h"

using namespace KTextEditor;

MovingRange::MovingRange ()
{
}

MovingRange::~MovingRange ()
{
}

void MovingRange::setRange (const Cursor &start, const Cursor &end)
{
  // just use other function, KTextEditor::Range will handle some normalization
  setRange (Range (start, end));
}

bool MovingRange::overlaps( const Range& range ) const
{
  if (range.start() <= start())
    return range.end() > start();

  else if (range.end() >= end())
    return range.start() < end();

  else
    return contains(range);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
