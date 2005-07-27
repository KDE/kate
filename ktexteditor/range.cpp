/* This file is part of the KDE libraries
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001, 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "range.h"

using namespace KTextEditor;

Range::Range()
  : m_start(new Cursor())
  , m_end(new Cursor())
{
}

Range::Range(const Cursor& start, const Cursor& end)
  : m_start(new Cursor(start))
  , m_end(new Cursor(end))
{
}

Range::Range(int startLine, int startCol, int endLine, int endCol)
  : m_start(new Cursor(startLine, startCol))
  , m_end(new Cursor(endLine, endCol))
{
}

Range::Range(Cursor* start, Cursor* end)
  : m_start(start)
  , m_end(end)
{
}

Range::~Range()
{
  delete m_start;
  delete m_end;
}

void Range::setStart(const Cursor& newStart)
{
  *m_start = newStart;
  if (start() > end())
    *m_end = newStart;
}

void Range::setEnd(const Cursor& newEnd)
{
  *m_end = newEnd;
  if (start() > end())
    *m_start = newEnd;
}

void Range::setRange(const Range& range)
{
  *m_start = range.start();
  *m_end = range.end();
}

void Range::setRange( const Cursor & start, const Cursor & end )
{
  if (start > end) {
    *m_start = end;
    *m_end = start;
  } else {
    *m_start = start;
    *m_end = end;
  }
}

bool Range::includesLine(int line) const
{
  return line >= start().line() && line <= end().line();
}

bool Range::includesColumn( int col ) const
{
  return start().column() <= col && end().column() > col;
}

int Range::includes( const Cursor& cursor ) const
{
  return ((cursor < start()) ? -1 : ((cursor > end()) ? 1:0));
}

bool Range::contains( const Range& range ) const
{
  return range.start() >= start() && range.end() <= end();
}

bool Range::overlaps( const Range& range ) const
{
  if (range.start() <= start())
    return range.end() > start();

  else if (range.end() >= end())
    return range.start() < end();

  else
    return contains(range);
}

bool Range::boundaryAt(const KTextEditor::Cursor& cursor) const
{
  return cursor == start() || cursor == end();
}

bool Range::boundaryOnLine(int line) const
{
  return start().line() == line || end().line() == line;
}


// kate: space-indent on; indent-width 2; replace-tabs on;
