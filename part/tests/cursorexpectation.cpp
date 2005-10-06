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

#include <math.h>

#include <QtTest/qttest_kde.h>
#include <kdebug.h>

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

  if (deletedBefore) {
    signalReceived(CharacterDeletedBefore);

  } else {
    signalReceived(CharacterDeletedAfter);
  }
}

void CursorExpectation::characterInserted( KTextEditor::SmartCursor * cursor, bool insertedBefore )
{
  COMPARE(cursor, m_smartCursor);

  if (insertedBefore) {
    signalReceived(CharacterInsertedBefore);

  } else {
    signalReceived(CharacterInsertedAfter);
  }
}

void CursorExpectation::positionChanged( KTextEditor::SmartCursor * cursor )
{
  COMPARE(cursor, m_smartCursor);
  signalReceived(PositionChanged);
}

void CursorExpectation::positionDeleted( KTextEditor::SmartCursor * cursor )
{
  COMPARE(cursor, m_smartCursor);
  signalReceived(PositionDeleted);
}

QString CursorExpectation::nameForSignal( int signal ) const
{
  switch (signal) {
    case CharacterDeletedBefore:
      return "a character to be deleted before cursor";
    case CharacterDeletedAfter:
      return "a character to be deleted after cursor";
    case CharacterInsertedBefore:
      return "a character to be inserted before cursor";
    case CharacterInsertedAfter:
      return "a character to be inserted after cursor";
    case PositionChanged:
      return "the cursor's position change";
    case PositionDeleted:
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

void CursorExpectation::signalReceived( int signal )
{
  COMPARE(*static_cast<KTextEditor::Cursor*>(m_smartCursor), m_expectedCursor);

  VERIFY(m_expectations & signal);

  signal = int(log(signal) / log(2));

  if (sender())
    m_watcherNotifications[signal]++;
  else
    m_notifierNotifications[signal]++;
}

#include "cursorexpectation.moc"
