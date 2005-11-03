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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katesmartcursor.h"

#include "katedocument.h"
#include "katesmartmanager.h"
#include "katesmartcursornotifier.h"

#include <kdebug.h>

KateSmartCursor::KateSmartCursor(const KTextEditor::Cursor& position, KTextEditor::Document* doc, bool moveOnInsert)
  : KTextEditor::SmartCursor(position, doc, moveOnInsert)
  , m_feedbackEnabled(false)
  , m_oldGroupLineStart(-1)
  , m_lastPosition(position)
  , m_isInternal(false)
  , m_ignoreTranslation(false)
  , m_notifier(0L)
  , m_watcher(0L)
{
  if (position > kateDocument()->documentEnd()) {
    kdWarning() << k_funcinfo << "Attempted to set cursor position past end of document." << endl;
    m_line = -1;
    m_column = -1;
  }

  // Replace straight line number with smartgroup + line offset
  m_smartGroup = kateDocument()->smartManager()->groupForLine(m_line);
  m_line = m_line - m_smartGroup->startLine();
  m_smartGroup->joined(this);
}

KateSmartCursor::KateSmartCursor( KTextEditor::Document * doc, bool moveOnInsert )
  : KTextEditor::SmartCursor(KTextEditor::Cursor(), doc, moveOnInsert)
  , m_feedbackEnabled(false)
  , m_oldGroupLineStart(-1)
  , m_isInternal(false)
  , m_ignoreTranslation(false)
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
  if (m_notifier) {
    emit m_notifier->deleted(this);
    delete m_notifier;
  }

  if (m_watcher)
    m_watcher->deleted(this);

  if (!kateDocument()->smartManager()->isClearing())
    m_smartGroup->leaving(this);
}

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
  setPositionInternal(KTextEditor::Cursor(_line, m_column), false);
}

void KateSmartCursor::setPositionInternal( const KTextEditor::Cursor & pos, bool internal )
{
  // Shortcut if there's no change :)
  if (*this == pos)
    return;

  KTextEditor::Cursor old = *this;

  // Remember this position if the feedback system needs it
  if (m_feedbackEnabled)
    m_lastPosition = *this;

  // Deal with crossing a smart group border
  bool haveToChangeGroups = !m_smartGroup->containsLine(pos.line());
  if (haveToChangeGroups) {
    m_smartGroup->leaving(this);
    m_smartGroup = kateDocument()->smartManager()->groupForLine(pos.line());
  }

  // Set the new position
  m_line = pos.line() - m_smartGroup->newStartLine();
  m_column = pos.column();

  // Finish dealing with crossing a smart group border
  if (haveToChangeGroups) {
    m_smartGroup->joined(this);
  }

  // Forget this position change if the feedback system doesn't need it
  if (!m_feedbackEnabled)
    m_lastPosition = *this;

  // Adjustments only needed for non-internal position changes...
  if (!internal)
    // Tell the range about this
    cursorChangedDirectly(old);
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
  if (*this < edit.start())
    return false;

  if (ignoreTranslation()) {
    setPositionInternal(invalid());
    return true;
  }

  // If this cursor is on a line affected by the edit
  if (edit.oldRange().overlapsLine(line())) {
    // If this cursor is at the start of the edit
    if (*this == edit.start()) {
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
        newPos = edit.start();

    } else {
      newPos = *this + edit.translate();
    }

    if (newPos != *this) {
      setPositionInternal(newPos);
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
  if (*this < edit.start() || ignoreTranslation()) {
    if (!range() || !static_cast<KateSmartRange*>(range())->feedbackEnabled())
      m_lastPosition = *this;
    return;
  }

  // We can rely on m_lastPosition because it is updated in translate(), otherwise just shifted() is called
  if (m_lastPosition != *this) {
    // position changed
    if (m_notifier)
      emit m_notifier->positionChanged(this);
    if (m_watcher)
      m_watcher->positionChanged(this);
  }

  if (!edit.oldRange().isEmpty() && edit.start() <= m_lastPosition && edit.oldRange().end() >= m_lastPosition) {
    if (edit.start() == m_lastPosition) {
      // character deleted after
      if (m_notifier)
        emit m_notifier->characterDeleted(this, false);
      if (m_watcher)
        m_watcher->characterDeleted(this, false);

    } else if (edit.oldRange().end() == m_lastPosition) {
      // character deleted before
      if (m_notifier)
        emit m_notifier->characterDeleted(this, true);
      if (m_watcher)
        m_watcher->characterDeleted(this, true);

    } else {
      // position deleted
      if (m_notifier)
        emit m_notifier->positionDeleted(this);
      if (m_watcher)
        m_watcher->positionDeleted(this);
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

  if (!range() || !static_cast<KateSmartRange*>(range())->feedbackEnabled())
    m_lastPosition = *this;
}

void KateSmartCursor::shifted( )
{
  Q_ASSERT(m_lastPosition != *this);

  // position changed
  if (m_notifier)
    emit m_notifier->positionChanged(this);
  if (m_watcher)
    m_watcher->positionChanged(this);

  if (!range() || !static_cast<KateSmartRange*>(range())->feedbackEnabled())
    m_lastPosition = *this;
}

void KateSmartCursor::migrate( KateSmartGroup * newGroup )
{
  int lineNum = line();
  m_smartGroup = newGroup;
  m_line = lineNum - m_smartGroup->startLine();
}

void KateSmartCursor::setPosition( const KTextEditor::Cursor & pos )
{
  if (pos > kateDocument()->documentEnd()) {
    kdWarning() << k_funcinfo << "Attempted to set cursor position past end of document." << endl;
    setPositionInternal(invalid(), false);
    return;
  }

  setPositionInternal(pos, false);
}

void KateSmartCursor::resetLastPosition( )
{
  m_lastPosition = *this;
}

bool KateSmartCursor::hasNotifier( ) const
{
  return m_notifier;
}

KTextEditor::SmartCursorWatcher * KateSmartCursor::watcher( ) const
{
  return m_watcher;
}

void KateSmartCursor::unbindFromRange( )
{
  setRange(0L);
}

void KateSmartCursor::setInternal( )
{
  m_isInternal = true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
