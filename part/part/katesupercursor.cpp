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

#include "katedocument.h"
#include "katesmartmanager.h"
#include "katesmartcursornotifier.h"

#include <kdebug.h>

KateSmartCursor::KateSmartCursor(const KTextEditor::Cursor& position, KTextEditor::Document* doc, bool moveOnInsert)
  : KTextEditor::SmartCursor(position, doc, moveOnInsert)
  , m_feedbackEnabled(false)
  , m_oldGroupLineStart(-1)
  , m_notifier(0L)
  , m_watcher(0L)
{
  // Replace straight line number with smartgroup + line offset
  m_smartGroup = kateDocument()->smartManager()->groupForLine(m_line);
  m_line = m_line - m_smartGroup->startLine();
  m_smartGroup->joined(this);
}

KateSmartCursor::KateSmartCursor( KTextEditor::Document * doc, bool moveOnInsert )
  : KTextEditor::SmartCursor(KTextEditor::Cursor(), doc, moveOnInsert)
  , m_feedbackEnabled(false)
  , m_oldGroupLineStart(-1)
  , m_notifier(0L)
  , m_watcher(0L)
{
  // Replace straight line number with smartgroup + line offset
  m_smartGroup = kateDocument()->smartManager()->groupForLine(m_line);
  m_line = m_line - m_smartGroup->startLine();
  m_smartGroup->joined(this);
}

KateSmartCursor::~KateSmartCursor()
{
  m_smartGroup->leaving(this);
  if (m_notifier)
    delete m_notifier;
}

/*void KateSmartCursor::editTextInserted(uint line, uint col, uint len)
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

void KateSmartCursor::editTextRemoved(uint line, uint col, uint len)
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

void KateSmartCursor::editLineWrapped(uint line, uint col, bool newLine)
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

void KateSmartCursor::editLineUnWrapped(uint line, uint col, bool removeLine, uint length)
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

void KateSmartCursor::editLineInserted (uint line)
{
  if (m_line >= int(line))
  {
    m_line++;

    emit positionChanged();
    return;
  }

  emit positionUnChanged();
}

void KateSmartCursor::editLineRemoved(uint line)
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
}*/

KateSmartCursor::operator QString()
{
  return QString("[%1,%1]").arg(line()).arg(column());
}

KateDocument* KateSmartCursor::kateDocument() const
{
  return static_cast<KateDocument*>(document());
}

bool KateSmartCursor::isValid( ) const
{
  return line() >= 0 && line() <= kateDocument()->lastLine() && column() >= 0 && column() <= kateDocument()->lineLength(line());
}

bool KateSmartCursor::isValid(const Cursor& position) const
{
  return position.line() >= 0 && position.line() <= kateDocument()->lastLine() && position.column() >= 0 && position.column() <= kateDocument()->lineLength(position.line());
}

bool KateSmartCursor::atEndOfLine( ) const
{
  return line() >= 0 && line() <= kateDocument()->lastLine() && column() >= kateDocument()->lineLength(line());
}

void KateSmartCursor::checkFeedback()
{
  bool feedbackNeeded = m_watcher || m_notifier;
  if (m_feedbackEnabled != feedbackNeeded) {
    m_smartGroup->changeCursorFeedback(this);
    m_feedbackEnabled = feedbackNeeded;
  }
}

int KateSmartCursor::line( ) const
{
  return m_smartGroup->startLine() + m_line;
}

void KateSmartCursor::setLine( int _line )
{
  bool haveToChangeGroups = !m_smartGroup->containsLine(_line);
  if (haveToChangeGroups) {
    m_smartGroup->leaving(this);
    m_smartGroup = kateDocument()->smartManager()->groupForLine(_line);
  }

  m_line = _line - m_smartGroup->startLine();

  if (haveToChangeGroups) {
    m_smartGroup->joined(this);
  }
}

void KateSmartCursor::setPositionInternal( const KTextEditor::Cursor & pos, bool internal )
{
  m_lastPosition = *this;

  bool haveToChangeGroups = !m_smartGroup->containsLine(pos.line());
  if (haveToChangeGroups) {
    m_smartGroup->leaving(this);
    m_smartGroup = kateDocument()->smartManager()->groupForLine(pos.line());
  }

  m_line = pos.line() - m_smartGroup->newStartLine();
  m_column = pos.column();

  if (haveToChangeGroups) {
    m_smartGroup->joined(this);
  }
}

KTextEditor::SmartCursorNotifier* KateSmartCursor::notifier( )
{
  if (!m_notifier) {
    m_notifier = new KateSmartCursorNotifier();
    checkFeedback();
  }
  return m_notifier;
}

void KateSmartCursor::deleteNotifier( )
{
  delete m_notifier;
  m_notifier = 0L;
  checkFeedback();
}

void KateSmartCursor::setWatcher( KTextEditor::SmartCursorWatcher * watcher )
{
  m_watcher = watcher;
  checkFeedback();
}

bool KateSmartCursor::translate( const KateEditInfo & edit )
{
  // If this cursor is before the edit, no action is required
  if (*this < edit.oldRange().start())
    return false;

  // If this cursor is on a line affected by the edit
  if (edit.oldRange().includesLine(line())) {
    // If this cursor is at the start of the edit
    if (*this == edit.oldRange().start()) {
      // And it doesn't need to move, no action is required
      if (!moveOnInsert())
        return false;
    }

    // Calculate the new position
    KTextEditor::Cursor newPos;
    if (edit.oldRange().contains(*this)) {
      if (moveOnInsert())
        newPos = edit.newRange().end();
      else
        newPos = edit.oldRange().start();

    } else {
      newPos = *this + edit.translate();
    }

    if (newPos != *this) {
      setPosition(newPos);
      return true;
    }

    return false;
  }

  // just need to adjust line number
  setLineInternal(line() + edit.translate().line());
  return true;
}

bool KateSmartCursor::cursorMoved( ) const
{
  bool ret = m_oldGroupLineStart != m_smartGroup->startLine();
  m_oldGroupLineStart = m_smartGroup->startLine();
  return ret;
}

void KateSmartCursor::setLineInternal( int newLine, bool internal )
{
  setPositionInternal(KTextEditor::Cursor(newLine, column()), internal);
}

void KateSmartCursor::translated(const KateEditInfo & edit)
{
  if (*this < edit.oldRange().start())
    return;

  if (!edit.oldRange().isEmpty() && edit.oldRange().contains(lastPosition())) {
    if (edit.oldRange().start() == lastPosition()) {
      // character deleted after
      if (m_notifier)
        emit m_notifier->characterDeleted(this, false);
      if (m_watcher)
        m_watcher->characterDeleted(this, false);

    } else if (edit.oldRange().end() == lastPosition()) {
      // character deleted before
      if (m_notifier)
        emit m_notifier->characterDeleted(this, true);
      if (m_watcher)
        m_watcher->characterDeleted(this, true);

    } else {
      // position deleted
      if (m_notifier)
        emit m_notifier->positionDeleted(this);
    }
  }

  if (!edit.newRange().isEmpty()) {
    if (*this == edit.newRange().start()) {
      // character inserted after
      if (m_notifier)
        emit m_notifier->characterInserted(this, false);
      if (m_watcher)
        m_watcher->characterInserted(this, false);

    } else if (*this == edit.newRange().end()) {
      // character inserted before
      if (m_notifier)
        emit m_notifier->characterInserted(this, true);
      if (m_watcher)
        m_watcher->characterInserted(this, true);
    }
  }
}

void KateSmartCursor::migrate( KateSmartGroup * newGroup )
{
  int lineNum = line();
  m_smartGroup = newGroup;
  m_line = lineNum - m_smartGroup->startLine();
}

void KateSmartCursor::setPosition( const KTextEditor::Cursor & pos )
{
  setPositionInternal(pos, false);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
