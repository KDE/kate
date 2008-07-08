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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "cursorexpectation.h"

#include <math.h>

#include <qtest_kde.h>
#include <kdebug.h>
#include <ktexteditor/smartcursor.h>

#include "kateregression.h"

CursorExpectation::CursorExpectation(KTextEditor::Cursor* cursor, CursorSignals signalsExpected, const KTextEditor::Cursor& positionExpected)
  : m_smartCursor(dynamic_cast<KTextEditor::SmartCursor*>(cursor))
  , m_expectedCursor(positionExpected)
  , m_expectations(signalsExpected)
  , m_smartCursorDeleted(false)
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
  connect(m_smartCursor->notifier(), SIGNAL(deleted(KTextEditor::SmartCursor*)),                  SLOT(deleted(KTextEditor::SmartCursor*)));

  m_smartCursor->setWatcher(this);

  KateRegression::self()->addCursorExpectation(this);
}

CursorExpectation::~CursorExpectation()
{
  if (!m_smartCursorDeleted) {
    m_smartCursor->setWatcher(0L);
    m_smartCursor->deleteNotifier();
  }
}

void CursorExpectation::characterDeleted( KTextEditor::SmartCursor * cursor, bool deletedBefore )
{
  QCOMPARE(cursor, m_smartCursor);

  if (deletedBefore) {
    signalReceived(CharacterDeletedBefore);

  } else {
    signalReceived(CharacterDeletedAfter);
  }
}

void CursorExpectation::characterInserted( KTextEditor::SmartCursor * cursor, bool insertedBefore )
{
  QCOMPARE(cursor, m_smartCursor);

  if (insertedBefore) {
    signalReceived(CharacterInsertedBefore);

  } else {
    signalReceived(CharacterInsertedAfter);
  }
}

void CursorExpectation::positionChanged( KTextEditor::SmartCursor * cursor )
{
  QCOMPARE(cursor, m_smartCursor);
  signalReceived(PositionChanged);
}

void CursorExpectation::positionDeleted( KTextEditor::SmartCursor * cursor )
{
  QCOMPARE(cursor, m_smartCursor);
  signalReceived(PositionDeleted);
}

void CursorExpectation::deleted( KTextEditor::SmartCursor * cursor )
{
  QCOMPARE(cursor, m_smartCursor);
  signalReceived(Deleted);
  m_smartCursorDeleted = true;
}

QString CursorExpectation::nameForSignal( int signal ) const
{
  switch (signal) {
    case CharacterDeletedBefore:
      return "a character to be deleted before the cursor";
    case CharacterDeletedAfter:
      return "a character to be deleted after the cursor";
    case CharacterInsertedBefore:
      return "a character to be inserted before the cursor";
    case CharacterInsertedAfter:
      return "a character to be inserted after the cursor";
    case PositionChanged:
      return "the cursor's position change";
    case PositionDeleted:
      return "the cursor's position deleted";
    case Deleted:
      return "the cursor being deleted";
    default:
      return "[invalid signal]";
  }
}

void CursorExpectation::checkExpectationsFulfilled( ) const
{
  bool fulfilled = true;
  for (int i = 0; i < numSignals; ++i) {
    int j = 1 << i;
    int countExpected = (m_expectations & j) ? 1 : 0;

    if (m_notifierNotifications[i] < countExpected)
      { fulfilled = false; kDebug() << "Notifier: Expected to be notified of" << nameForSignal(j); }
    else if (m_notifierNotifications[i] > countExpected)
      if (countExpected)
        { fulfilled = false; kDebug() << "Notifier: Notified more than once about" << nameForSignal(j); }
      else
        { fulfilled = false; kDebug() << "Notifier: Notified incorrectly about" << nameForSignal(j); }

    if (m_watcherNotifications[i] < countExpected)
      { fulfilled = false; kDebug() << "Watcher: Expected to be notified of" << nameForSignal(j); }
    else if (m_watcherNotifications[i] > countExpected)
      if (countExpected)
        { fulfilled = false; kDebug() << "Watcher: Notified more than once about" << nameForSignal(j); }
      else
        { fulfilled = false; kDebug() << "Watcher: Notified incorrectly about" << nameForSignal(j); }
  }
  QVERIFY(fulfilled);
}

void CursorExpectation::signalReceived( int signal )
{
  QCOMPARE(*static_cast<KTextEditor::Cursor*>(m_smartCursor), m_expectedCursor);

  QVERIFY(m_expectations & signal);

  // Can't think of the algorithm right now
  switch (signal) {
    case CharacterDeletedBefore:
      signal = 0;
      break;
    case CharacterDeletedAfter:
      signal = 1;
      break;
    case CharacterInsertedBefore:
      signal = 2;
      break;
    case CharacterInsertedAfter:
      signal = 3;
      break;
    case PositionChanged:
      signal = 4;
      break;
    case PositionDeleted:
      signal = 5;
      break;
    case Deleted:
      signal = 6;
      break;
  }

  if (!sender())
    m_watcherNotifications[signal]++;
  else
    m_notifierNotifications[signal]++;
}

#include "cursorexpectation.moc"
