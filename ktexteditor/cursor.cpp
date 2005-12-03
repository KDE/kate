/* This file is part of the KDE project
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>

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

#include "cursor.h"

#include "range.h"

using namespace KTextEditor;

Cursor::Cursor( )
  : m_line(0)
  , m_column(0)
  , m_range(0L)
{
}

Cursor::Cursor( int _line, int _column )
  : m_line(_line)
  , m_column(_column)
  , m_range(0L)
{
}

Cursor::Cursor(const Cursor& copy)
  : m_line(copy.line())
  , m_column(copy.column())
  , m_range(0L)
{
}

bool Cursor::isValid() const
{
  return m_line >= 0 && m_column >= 0;
}

const Cursor & Cursor::invalid( )
{
  static Cursor invalid(-1,-1);
  return invalid;
}

const Cursor& Cursor::start()
{
  static Cursor start(0, 0);
  return start;
}

void Cursor::setLine( int line )
{
  if (line == this->line())
    return;

  Cursor old = *this;

  m_line = line;

  cursorChangedDirectly(old);
}

void Cursor::setColumn( int column )
{
  if (column == this->column())
    return;

  Cursor old = *this;

  m_column = column;

  cursorChangedDirectly(old);
}

void Cursor::setPosition( const Cursor & pos )
{
  if (pos == *this)
    return;

  Cursor old = *this;

  m_line = pos.line();
  m_column = pos.column();

  cursorChangedDirectly(old);
}

bool Cursor::isSmartCursor( ) const
{
  return false;
}

int Cursor::line( ) const
{
  return m_line;
}

void Cursor::position (int &_line, int &_column) const
{
  _line = line(); _column = column();
}

Cursor::~ Cursor( )
{
}

void Cursor::setRange( Range * range )
{
  m_range = range;
}

void KTextEditor::Cursor::cursorChangedDirectly(const Cursor& from)
{
  if (m_range) {
    if (this == &m_range->start())
      m_range->rangeChanged(this, Range(from, m_range->end()));
    else
      m_range->rangeChanged(this, Range(m_range->start(), from));
  }
}

bool KTextEditor::Cursor::atStartOfLine( ) const
{
  return m_column == 0;
}

bool KTextEditor::Cursor::atStartOfDocument( ) const
{
  return m_line == 0 && m_column == 0;
}

SmartCursor * KTextEditor::Cursor::toSmartCursor( ) const
{
  return 0L;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
