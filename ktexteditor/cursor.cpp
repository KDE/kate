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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "cursor.h"

#include "document.h"

using namespace KTextEditor;

Cursor::Cursor( )
  : m_line(0)
  , m_column(0)
{
}

Cursor::Cursor( int _line, int _column )
  : m_line(_line)
  , m_column(_column)
{
}

Cursor::Cursor(const Cursor& copy)
  : m_line(copy.line())
  , m_column(copy.column())
{
}

bool KTextEditor::Cursor::isValid() const
{
  return m_line >= 0 && m_column >= 0;
}

const Cursor & KTextEditor::Cursor::invalid( )
{
  static Cursor invalid(-1,-1);
  return invalid;
}

void Cursor::setPosition( const Cursor & pos )
{
  m_line = pos.line();
  m_column = pos.column();
}

bool Cursor::isSmart( ) const
{
  return false;
}

int Cursor::line( ) const
{
  return m_line;
}

void Cursor::setColumn( int _column )
{
  m_column = _column;
}

void Cursor::setLine( int _line )
{
  m_line = _line;
}

void Cursor::position (int &_line, int &_column) const
{
  _line = line(); _column = column();
}

Cursor::~ Cursor( )
{
}

SmartCursor::~ SmartCursor( )
{
}

SmartCursor::SmartCursor( const Cursor & position, Document * doc, bool moveOnInsert )
  : Cursor(position)
  , m_doc(doc)
  , m_moveOnInsert(moveOnInsert)
  , m_range(0L)
{
  Q_ASSERT(m_doc);
}

bool SmartCursor::atEndOfDocument( ) const
{
  return *this >= m_doc->end();
}

bool KTextEditor::SmartCursor::isSmart( ) const
{
  return true;
}

KTextEditor::SmartCursorWatcher::~ SmartCursorWatcher( )
{
}

void KTextEditor::SmartCursorWatcher::positionChanged( SmartCursor * )
{
}

void KTextEditor::SmartCursorWatcher::positionDeleted( SmartCursor * )
{
}

void KTextEditor::SmartCursorWatcher::characterDeleted( SmartCursor * , bool )
{
}

void KTextEditor::SmartCursorWatcher::characterInserted( SmartCursor * , bool )
{
}

#include "cursor.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
