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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "range.h"

using namespace KTextEditor;

Range::Range()
  : m_start(new Cursor())
  , m_end(new Cursor())
{
  m_start->setRange(this);
  m_end->setRange(this);
}

Range::Range(const Cursor& start, const Cursor& end)
{
  if (start <= end) {
    m_start = new Cursor(start);
    m_end = new Cursor(end);

  } else {
    m_start = new Cursor(end);
    m_end = new Cursor(start);
  }

  m_start->setRange(this);
  m_end->setRange(this);
}

Range::Range(const Cursor& start, int width)
  : m_start(new Cursor(start))
  , m_end(new Cursor(start.line(), start.column() + width))
{
  m_start->setRange(this);
  m_end->setRange(this);
}

Range::Range(const Cursor& start, int endLine, int endColumn)
  : m_start(new Cursor(start))
  , m_end(new Cursor(endLine, endColumn))
{
  if (*m_end < *m_start) {
    Cursor* temp = m_end;
    m_end = m_start;
    m_start = temp;
  }

  m_start->setRange(this);
  m_end->setRange(this);
}

Range::Range(int startLine, int startColumn, int endLine, int endColumn)
  : m_start(new Cursor(startLine, startColumn))
  , m_end(new Cursor(endLine, endColumn))
{
  if (*m_end < *m_start) {
    Cursor* temp = m_end;
    m_end = m_start;
    m_start = temp;
  }

  m_start->setRange(this);
  m_end->setRange(this);
}

Range::Range(Cursor* start, Cursor* end)
  : m_start(start)
  , m_end(end)
{
  if (*m_end < *m_start) {
    Cursor temp = *m_end;
    *m_end = *m_start;
    *m_start = temp;
  }

  m_start->setRange(this);
  m_end->setRange(this);
}

Range::Range(const Range& copy)
  : m_start(new Cursor(copy.start()))
  , m_end(new Cursor(copy.end()))
{
  m_start->setRange(this);
  m_end->setRange(this);
}

Range::~Range()
{
  delete m_start;
  delete m_end;
}

bool Range::isValid( ) const
{
  return start().isValid() && end().isValid();
}

const Range& Range::invalid()
{
  static Range r;
  if (r.start().line() != -1)
    r.setRange(Cursor(-1,-1),Cursor(-1,-1));
  return r;
}

void Range::setRange(const Range& range)
{
  *m_start = range.start();
  *m_end = range.end();
}

void Range::setRange( const Cursor & start, const Cursor & end )
{
  if (start > end)
    setRange(Range(end, start));
  else
    setRange(Range(start, end));
}

bool Range::containsLine(int line) const
{
  return (line > start().line() || line == start().line() && !start().column()) && line < end().line();
}

bool Range::overlapsLine(int line) const
{
  return line >= start().line() && line <= end().line();
}

bool Range::overlapsColumn( int col ) const
{
  return start().column() <= col && end().column() > col;
}

bool Range::contains( const Cursor& cursor ) const
{
  return cursor >= start() && cursor < end();
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

bool Range::boundaryAtCursor(const Cursor& cursor) const
{
  return cursor == start() || cursor == end();
}

bool Range::boundaryOnLine(int line) const
{
  return start().line() == line || end().line() == line;
}

bool Range::confineToRange(const Range& range)
{
  if (start() < range.start())
    if (end() > range.end())
      setRange(range);
    else
      start() = range.start();
  else if (end() > range.end())
    end() = range.end();
  else
    return false;

  return true;
}

bool Range::expandToRange(const Range& range)
{
  if (start() > range.start())
    if (end() < range.end())
      setRange(range);
    else
      start() = range.start();
  else if (end() < range.end())
    end() = range.end();
  else
    return false;

  return true;
}

void Range::rangeChanged( Cursor * c, const Range& )
{
  if (c == m_start) {
    if (*c > *m_end)
      *m_end = *c;

  } else if (c == m_end) {
    if (*c < *m_start)
      *m_start = *c;
  }
}

void Range::setBothLines( int line )
{
  setRange(Range(line, start().column(), line, end().column()));
}

bool KTextEditor::Range::onSingleLine( ) const
{
  return start().line() == end().line();
}

int KTextEditor::Range::columnWidth( ) const
{
  return end().column() - start().column();
}

int KTextEditor::Range::numberOfLines( ) const
{
  return end().line() - start().line();
}

bool KTextEditor::Range::isEmpty( ) const
{
  return start() == end();
}

int Range::positionRelativeToCursor( const Cursor & cursor ) const
{
  if (end() <= cursor)
    return -1;

  if (start() > cursor)
    return +1;

  return 0;
}

int Range::positionRelativeToLine( int line ) const
{
  if (end().line() < line)
    return -1;

  if (start().line() > line)
    return +1;

  return 0;
}

void KTextEditor::Range::setBothColumns( int column )
{
  setRange(Range(start().line(), column, end().line(), column));
}

bool KTextEditor::Range::isSmartRange( ) const
{
  return false;
}

SmartRange* KTextEditor::Range::toSmartRange( ) const
{
  return 0L;
}

Range KTextEditor::Range::intersect( const Range & range ) const
{
  if (!isValid() || !range.isValid() || *this > range || *this < range)
    return invalid();

  return Range(qMax(start(), range.start()), qMin(end(), range.end()));
}

Range KTextEditor::Range::encompass( const Range & range ) const
{
  if (!isValid())
    if (range.isValid())
      return range;
    else
      return invalid();
  else if (!range.isValid())
    return *this;
  else
    return Range(qMin(start(), range.start()), qMax(end(), range.end()));
}

// kate: space-indent on; indent-width 2; replace-tabs on;
