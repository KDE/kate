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

#include "smartcursor.h"

#include "document.h"
#include "smartrange.h"

using namespace KTextEditor;

SmartCursor::SmartCursor( const Cursor & position, Document * doc, InsertBehaviour insertBehaviour )
  : Cursor(position)
  , m_doc(doc)
  , m_moveOnInsert(insertBehaviour == MoveOnInsert)
{
  Q_ASSERT(m_doc);
}

SmartCursor::~ SmartCursor( )
{
}

bool SmartCursor::isValid( ) const
{
  return m_doc->cursorInText(*this);
}

bool SmartCursor::atEndOfDocument( ) const
{
  return *this >= m_doc->documentEnd();
}

bool SmartCursor::isSmartCursor( ) const
{
  return true;
}

bool SmartCursor::insertText( const QStringList & text, bool block )
{
  return document()->insertText(*this, text, block);
}

QChar SmartCursor::character( ) const
{
  return document()->character(*this);
}

SmartRange * SmartCursor::smartRange( ) const
{
  return static_cast<SmartRange*>(m_range);
}

bool SmartCursor::atEndOfLine( ) const
{
  return column() == m_doc->lineLength(line());
}

SmartCursor::InsertBehaviour SmartCursor::insertBehaviour( ) const
{
  return m_moveOnInsert ? MoveOnInsert : StayOnInsert;
}

void SmartCursor::setInsertBehaviour( InsertBehaviour insertBehaviour )
{
  m_moveOnInsert = insertBehaviour == MoveOnInsert;
}

SmartCursor * SmartCursor::toSmartCursor( ) const
{
  return const_cast<SmartCursor*>(this);
}

bool SmartCursor::advance(int distance, AdvanceMode mode)
{
  Cursor c = *this;
  if (mode == ByCharacter) {
    while (distance) {
      int lineLength = document()->lineLength(c.line());

      if (distance > 0) {
        int advance = qMin(lineLength - c.column(), distance);

        if (distance > advance) {
          if (c.line() + 1 >= document()->lines())
            return false;

          c.setPosition(c.line() + 1, 0);
          // Account for end of line advancement
          distance -= advance + 1;

        } else {
          c.setColumn(c.column() + distance);
          distance = 0;
        }

      } else {
        int back = qMin(c.column(), -distance);
        if (-distance > back) {
          if (c.line() - 1 < 0)
            return false;

          c.setPosition(c.line() - 1, document()->lineLength(c.line() - 1));
          // Account for end of line advancement
          distance += back + 1;

        } else {
          c.setColumn(c.column() + distance);
          distance = 0;
        }
      }
    }

  } else {
    // Not supported by the interface alone
    return false;
  }

  setPosition(c);
  return true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
