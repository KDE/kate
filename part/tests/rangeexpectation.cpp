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

#include "rangeexpectation.h"

#include <QtTest/qttest_kde.h>

#include "kateregression.h"

RangeExpectation::RangeExpectation(KTextEditor::Range* range)
  : m_smartRange(dynamic_cast<KTextEditor::SmartRange*>(range))
{
  Q_ASSERT(m_smartRange);
  for (int i = 0; i < numSignals; ++i) {
    m_expectations[i] = false;
    m_notifierNotifications[i] = 0;
    m_watcherNotifications[i] = 0;
  }

  connect(m_smartRange->notifier(), SIGNAL(positionChanged(KTextEditor::SmartRange*)),        SLOT(positionChanged(KTextEditor::SmartRange*)));
  connect(m_smartRange->notifier(), SIGNAL(contentsChanged(KTextEditor::SmartRange*)),        SLOT(contentsChanged(KTextEditor::SmartRange*)));
  connect(m_smartRange->notifier(), SIGNAL(boundaryDeleted(KTextEditor::SmartRange*,bool)),   SLOT(boundaryDeleted(KTextEditor::SmartRange*,bool)));
  connect(m_smartRange->notifier(), SIGNAL(eliminated(KTextEditor::SmartRange*)),             SLOT(eliminated(KTextEditor::SmartRange*)));
  connect(m_smartRange->notifier(), SIGNAL(firstCharacterDeleted(KTextEditor::SmartRange*)),  SLOT(firstCharacterDeleted(KTextEditor::SmartRange*)));
  connect(m_smartRange->notifier(), SIGNAL(lastCharacterDeleted(KTextEditor::SmartRange*)),   SLOT(lastCharacterDeleted(KTextEditor::SmartRange*)));

  m_smartRange->setWatcher(this);

  KateRegression::self()->addRangeExpectation(this);
}

RangeExpectation::~ RangeExpectation( )
{
  m_smartRange->setWatcher(0L);
  m_smartRange->deleteNotifier();
}

void RangeExpectation::checkExpectationsFulfilled( ) const
{
  for (int i = 0; i < numSignals; ++i) {
    if (m_expectations[i]) {
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

void RangeExpectation::setExpected( int signal )
{
  Q_ASSERT(signal >= 0 && signal < numSignals);
  m_expectations[signal] = true;
}

void RangeExpectation::positionChanged( KTextEditor::SmartRange * )
{
  VERIFY(m_expectations[signalPositionChanged]);
  COMPARE(*static_cast<KTextEditor::Range*>(m_smartRange), m_expectedRange);

  if (sender())
    m_watcherNotifications[signalPositionChanged]++;
  else
    m_notifierNotifications[signalPositionChanged]++;
}

void RangeExpectation::contentsChanged( KTextEditor::SmartRange * )
{
  VERIFY(m_expectations[signalContentsChanged]);
  COMPARE(*static_cast<KTextEditor::Range*>(m_smartRange), m_expectedRange);

  if (sender())
    m_watcherNotifications[signalContentsChanged]++;
  else
    m_notifierNotifications[signalContentsChanged]++;
}

void RangeExpectation::boundaryDeleted( KTextEditor::SmartRange * , bool start )
{
  COMPARE(*static_cast<KTextEditor::Range*>(m_smartRange), m_expectedRange);

  if (start) {
    VERIFY(m_expectations[signalStartBoundaryDeleted]);
    if (sender())
      m_watcherNotifications[signalStartBoundaryDeleted]++;
    else
      m_notifierNotifications[signalStartBoundaryDeleted]++;

  } else {
    VERIFY(m_expectations[signalEndBoundaryDeleted]);
    if (sender())
      m_watcherNotifications[signalEndBoundaryDeleted]++;
    else
      m_notifierNotifications[signalEndBoundaryDeleted]++;
  }
}

void RangeExpectation::eliminated( KTextEditor::SmartRange * )
{
  VERIFY(m_expectations[signalEliminated]);
  COMPARE(*static_cast<KTextEditor::Range*>(m_smartRange), m_expectedRange);

  if (sender())
    m_watcherNotifications[signalEliminated]++;
  else
    m_notifierNotifications[signalEliminated]++;
}

void RangeExpectation::firstCharacterDeleted( KTextEditor::SmartRange * )
{
  VERIFY(m_expectations[signalFirstCharacterDeleted]);
  COMPARE(*static_cast<KTextEditor::Range*>(m_smartRange), m_expectedRange);

  if (sender())
    m_watcherNotifications[signalFirstCharacterDeleted]++;
  else
    m_notifierNotifications[signalFirstCharacterDeleted]++;
}

void RangeExpectation::lastCharacterDeleted( KTextEditor::SmartRange * )
{
  VERIFY(m_expectations[signalLastCharacterDeleted]);
  COMPARE(*static_cast<KTextEditor::Range*>(m_smartRange), m_expectedRange);

  if (sender())
    m_watcherNotifications[signalLastCharacterDeleted]++;
  else
    m_notifierNotifications[signalLastCharacterDeleted]++;
}

QString RangeExpectation::nameForSignal( int signal ) const
{
  switch (signal) {
    case signalPositionChanged:
      return "position change";
    case signalContentsChanged:
      return "content change";
    case signalStartBoundaryDeleted:
      return "starting boundary deletion";
    case signalEndBoundaryDeleted:
      return "ending boundary deletion";
    case signalEliminated:
      return "elimination of the range";
    case signalFirstCharacterDeleted:
      return "first character deletion";
    case signalLastCharacterDeleted:
      return "last character deletion";
    default:
      return "[invalid signal]";
  }
}

void RangeExpectation::setExpected( const KTextEditor::Range & expectedRange )
{
  m_expectedRange = expectedRange;
}

#include "rangeexpectation.moc"
