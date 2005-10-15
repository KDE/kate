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

bool KTextEditor::Cursor::isValid() const
{
  return m_line >= 0 && m_column >= 0;
}

const Cursor & KTextEditor::Cursor::invalid( )
{
  static Cursor invalid(-1,-1);
  return invalid;
}

const Cursor& Cursor::start()
{
  static Cursor start(0, 0);
  return start;
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
{
  Q_ASSERT(m_doc);
}

bool SmartCursor::atEndOfDocument( ) const
{
  return *this >= m_doc->documentEnd();
}

bool KTextEditor::SmartCursor::isSmart( ) const
{
  return true;
}

bool KTextEditor::SmartCursor::insertText( const QStringList & text, bool block )
{
  return document()->insertText(*this, text, block);
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

QChar KTextEditor::SmartCursor::character( ) const
{
  return document()->character(*this);
}

SmartCursorWatcher::SmartCursorWatcher( )
  : m_wantDirectChanges(true)
{
}

bool KTextEditor::SmartCursorWatcher::wantsDirectChanges( ) const
{
  return m_wantDirectChanges;
}

void KTextEditor::SmartCursorWatcher::setWantsDirectChanges( bool wantsDirectChanges )
{
  m_wantDirectChanges = wantsDirectChanges;
}

SmartRange * KTextEditor::SmartCursor::smartRange( ) const
{
  return static_cast<SmartRange*>(m_range);
}

void KTextEditor::Cursor::setRange( Range * range )
{
  m_range = range;
}

void KTextEditor::SmartCursor::setRange( SmartRange * range )
{
  Cursor::setRange(range);
  checkFeedback();
}

void KTextEditor::SmartCursor::setLine( int line )
{
  if (line == this->line())
    return;

  Cursor::setLine(line);
  if (m_range)
    m_range->cursorChanged(this);
}

void KTextEditor::SmartCursor::setColumn( int column )
{
  if (column == this->column())
    return;

  Cursor::setColumn(column);
  if (m_range)
    m_range->cursorChanged(this);
}

void KTextEditor::SmartCursor::setPosition( const Cursor & pos )
{
  if (pos == *this)
    return;

  Cursor::setPosition(pos);
  if (m_range)
    m_range->cursorChanged(this);
}

KTextEditor::SmartCursorNotifier::SmartCursorNotifier( )
  : m_wantDirectChanges(true)
{
}

bool KTextEditor::SmartCursorNotifier::wantsDirectChanges( ) const
{
  return m_wantDirectChanges;
}

void KTextEditor::SmartCursorNotifier::setWantsDirectChanges( bool wantsDirectChanges )
{
  m_wantDirectChanges = wantsDirectChanges;
}

#include "cursor.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
