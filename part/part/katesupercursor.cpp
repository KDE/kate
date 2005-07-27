/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katesupercursor.h"
#include "katesupercursor.moc"

#include "katedocument.h"

#include <kdebug.h>

#include <qobject.h>

KateSuperCursor::KateSuperCursor(KateDocument* doc, bool privateC, const KTextEditor::Cursor& cursor, QObject* parent, const char* name)
  : QObject(parent, name)
  , KateDocCursor(cursor.line(), cursor.column(), doc)
  , m_doc (doc)
{
  m_moveOnInsert = false;
  m_lineRemoved = false;
  m_privateCursor = privateC;

  connect(this, SIGNAL(positionDirectlyChanged()), SIGNAL(positionChanged()));

  m_doc->addSuperCursor (this, privateC);
}

KateSuperCursor::KateSuperCursor(KateDocument* doc, bool privateC, int lineNum, int col, QObject* parent, const char* name)
  : QObject(parent, name)
  , KateDocCursor(lineNum, col, doc)
  , m_doc (doc)
{
  m_moveOnInsert = false;
  m_lineRemoved = false;
  m_privateCursor = privateC;

  connect(this, SIGNAL(positionDirectlyChanged()), SIGNAL(positionChanged()));

  m_doc->addSuperCursor (this, privateC);
}

KateSuperCursor::~KateSuperCursor ()
{
  m_doc->removeSuperCursor (this, m_privateCursor);
}

bool KateSuperCursor::insertText(const QString& s)
{
  return KateDocCursor::insertText(s);
}

bool KateSuperCursor::removeText(uint nbChar)
{
  return KateDocCursor::removeText(nbChar);
}

QChar KateSuperCursor::currentChar() const
{
  return KateDocCursor::currentChar();
}

bool KateSuperCursor::atStartOfLine() const
{
  return column() == 0;
}

bool KateSuperCursor::atEndOfLine() const
{
  return column() >= (int)m_doc->kateTextLine(line())->length();
}

bool KateSuperCursor::moveOnInsert() const
{
  return m_moveOnInsert;
}

void KateSuperCursor::setMoveOnInsert(bool moveOnInsert)
{
  m_moveOnInsert = moveOnInsert;
}

void KateSuperCursor::setPosition(const KTextEditor::Cursor& pos)
{
  KTextEditor::Cursor tmp (pos);
  KateDocCursor::setPosition(pos);

  if (tmp != *this)
    emit positionDirectlyChanged();
}

KateDocument* KateSuperCursor::doc() const
{
  return m_doc;
}

void KateSuperCursor::editTextInserted(uint line, uint col, uint len)
{
  if (m_line == int(line))
  {
    if ((m_column > int(col)) || (m_moveOnInsert && (m_column == int(col))))
    {
      bool insertedAt = m_column == int(col);

      m_column += len;

      if (insertedAt)
        emit charInsertedAt();

      emit positionChanged();
      return;
    }
  }

  emit positionUnChanged();
}

void KateSuperCursor::editTextRemoved(uint line, uint col, uint len)
{
  if (m_line == int(line))
  {
    if (m_column > int(col))
    {
      if (m_column > int(col + len))
      {
        m_column -= len;
      }
      else
      {
        bool prevCharDeleted = m_column == int(col + len);

        m_column = col;

        if (prevCharDeleted)
          emit charDeletedBefore();
        else
          emit positionDeleted();
      }

      emit positionChanged();
      return;

    }
    else if (m_column == int(col))
    {
      emit charDeletedAfter();
    }
  }

  emit positionUnChanged();
}

void KateSuperCursor::editLineWrapped(uint line, uint col, bool newLine)
{
  if (newLine && (m_line > int(line)))
  {
    m_line++;

    emit positionChanged();
    return;
  }
  else if ( (m_line == int(line)) && (m_column > int(col)) || (m_moveOnInsert && (m_column == int(col))) )
  {
    m_line++;
    m_column -= col;

    emit positionChanged();
    return;
  }

  emit positionUnChanged();
}

void KateSuperCursor::editLineUnWrapped(uint line, uint col, bool removeLine, uint length)
{
  if (removeLine && (m_line > int(line+1)))
  {
    m_line--;

    emit positionChanged();
    return;
  }
  else if ( (m_line == int(line+1)) && (removeLine || (m_column < int(length))) )
  {
    m_line = line;
    m_column += col;

    emit positionChanged();
    return;
  }
  else if ( (m_line == int(line+1)) && (m_column >= int(length)) )
  {
    m_column -= length;

    emit positionChanged();
    return;
  }

  emit positionUnChanged();
}

void KateSuperCursor::editLineInserted (uint line)
{
  if (m_line >= int(line))
  {
    m_line++;

    emit positionChanged();
    return;
  }

  emit positionUnChanged();
}

void KateSuperCursor::editLineRemoved(uint line)
{
  if (m_line > int(line))
  {
    m_line--;

    emit positionChanged();
    return;
  }
  else if (m_line == int(line))
  {
    m_line = (int(line) <= m_doc->lastLine()) ? line : (line - 1);
    m_column = 0;

    emit positionDeleted();

    emit positionChanged();
    return;
  }

  emit positionUnChanged();
}

KateSuperCursor::operator QString()
{
  return QString("[%1,%1]").arg(line()).arg(column());
}

// kate: space-indent on; indent-width 2; replace-tabs on;
