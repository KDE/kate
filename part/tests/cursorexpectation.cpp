/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#include "cursorexpectation.h"

#include <QtTest/qttest_kde.h>

#include "kateregression.h"

CursorExpectation::CursorExpectation(KTextEditor::Cursor* cursor, CursorSignals signalsExpected, const KTextEditor::Cursor& positionExpected)
  : m_smartCursor(dynamic_cast<KTextEditor::SmartCursor*>(cursor))
  , m_expectedCursor(positionExpected)
  , m_expectations(signalsExpected)
{
  Q_ASSERT(m_smartCursor);
  for (int i = 0; i < numSignals; ++i) {
    m_notifierNotifications[i] = 0;
    m_watcherNotifications[i] = 0;
  }

  if (!m_expectedCursor.isValid())
    m_expectedCursor = *m_smartCursor;

  connect(m_smartCursor->notifier(), SIGNAL(characterDeleted(KTextEditor::SmartCursor*, bool)),   SLOT(characterDeleted(KTextEditor::SmartCursor*, bool)));
  connect(m_smartCursor->notifier(), SIGNAL(characterInserted(KTextEditor::SmartCursor*, bool)),  SLOT(characterInserted(KTextEditor::SmartCursor*, bool)));
  connect(m_smartCursor->notifier(), SIGNAL(positionChanged(KTextEditor::SmartCursor*)),          SLOT(positionChanged(KTextEditor::SmartCursor*)));
  connect(m_smartCursor->notifier(), SIGNAL(positionDeleted(KTextEditor::SmartCursor*)),          SLOT(positionDeleted(KTextEditor::SmartCursor*)));

  m_smartCursor->setWatcher(this);

  KateRegression::self()->addCursorExpectation(this);
}

CursorExpectation::~CursorExpectation()
{
  m_smartCursor->setWatcher(0L);
  m_smartCursor->deleteNotifier();
}

void CursorExpectation::characterDeleted( KTextEditor::SmartCursor * cursor, bool deletedBefore )
{
  COMPARE(cursor, m_smartCursor);
  COMPARE(*static_cast<KTextEditor::Cursor*>(cursor), m_expectedCursor);

  if (deletedBefore) {
    VERIFY(m_expectations & CharacterDeletedBefore);
    if (sender())
      m_watcherNotifications[iCharacterDeletedBefore]++;
    else
      m_notifierNotifications[iCharacterDeletedBefore]++;

  } else {
    VERIFY(m_expectations & CharacterDeletedAfter);
    if (sender())
      m_watcherNotifications[iCharacterDeletedAfter]++;
    else
      m_notifierNotifications[iCharacterDeletedAfter]++;
  }
}

void CursorExpectation::characterInserted( KTextEditor::SmartCursor * cursor, bool insertedBefore )
{
  COMPARE(cursor, m_smartCursor);
  COMPARE(*static_cast<KTextEditor::Cursor*>(cursor), m_expectedCursor);

  if (insertedBefore) {
    VERIFY(m_expectations & CharacterInsertedBefore);
    if (sender())
      m_watcherNotifications[iCharacterInsertedBefore]++;
    else
      m_notifierNotifications[iCharacterInsertedBefore]++;

  } else {
    VERIFY(m_expectations & CharacterInsertedAfter);
    if (sender())
      m_watcherNotifications[iCharacterInsertedAfter]++;
    else
      m_notifierNotifications[iCharacterInsertedAfter]++;
  }
}

void CursorExpectation::positionChanged( KTextEditor::SmartCursor * cursor )
{
  COMPARE(cursor, m_smartCursor);
  COMPARE(*static_cast<KTextEditor::Cursor*>(cursor), m_expectedCursor);

  VERIFY(m_expectations & PositionChanged);
  if (sender())
    m_watcherNotifications[iPositionChanged]++;
  else
    m_notifierNotifications[iPositionChanged]++;
}

void CursorExpectation::positionDeleted( KTextEditor::SmartCursor * cursor )
{
  COMPARE(cursor, m_smartCursor);
  COMPARE(*static_cast<KTextEditor::Cursor*>(cursor), m_expectedCursor);

  VERIFY(m_expectations & PositionDeleted);
  if (sender())
    m_watcherNotifications[iPositionDeleted]++;
  else
    m_notifierNotifications[iPositionDeleted]++;
}

QString CursorExpectation::nameForSignal( int signal ) const
{
  switch (signal) {
    case iCharacterDeletedBefore:
      return "a character to be deleted before cursor";
    case iCharacterDeletedAfter:
      return "a character to be deleted after cursor";
    case iCharacterInsertedBefore:
      return "a character to be inserted before cursor";
    case iCharacterInsertedAfter:
      return "a character to be inserted after cursor";
    case iPositionChanged:
      return "the cursor's position change";
    case iPositionDeleted:
      return "the cursor's position change";
    default:
      return "[invalid signal]";
  }
}

void CursorExpectation::checkExpectationsFulfilled( ) const
{
  for (int i = 0; i < numSignals; ++i) {
    int j = 2 << (i - 1);
    if (m_expectations & j) {
      if (m_notifierNotifications[i] == 0)
        FAIL(QString("Notifier: Expected to be notified of %1.").arg(nameForSignal(i)).toLatin1());
      else if (m_notifierNotifications[i] > 1)
        FAIL(QString("Notifier: Notified more than once about %1.").arg(nameForSignal(i)).toLatin1());

      if (m_watcherNotifications[i] == 0)
        FAIL(QString("Watcher: Expected to be notified of %1.").arg(nameForSignal(i)).toLatin1());
      else if (m_watcherNotifications[i] > 1)
        FAIL(QString("Watcher: Notified more than once about %1.").arg(nameForSignal(i)).toLatin1());
    }
  }
}

#include "cursorexpectation.moc"
