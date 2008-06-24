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

#include "rangeexpectation.h"

#include <math.h>

#include <qtest_kde.h>
#include <ktexteditor/smartrange.h>

#include "kateregression.h"

RangeExpectation::RangeExpectation(KTextEditor::Range* range, RangeSignals signalsExpected, const KTextEditor::Range& rangeExpected)
  : m_smartRange(dynamic_cast<KTextEditor::SmartRange*>(range))
  , m_expectedRange(rangeExpected)
  , m_expectations(signalsExpected)
{
  Q_ASSERT(m_smartRange);
  for (int i = 0; i < numSignals; ++i) {
    m_notifierNotifications[i] = 0;
    m_watcherNotifications[i] = 0;
  }

  // TODO could switch to auto connection
  connect(m_smartRange->primaryNotifier(), SIGNAL(rangePositionChanged(KTextEditor::SmartRange*)),        SLOT(rangePositionChanged(KTextEditor::SmartRange*)));
  connect(m_smartRange->primaryNotifier(), SIGNAL(rangeContentsChanged(KTextEditor::SmartRange*)),        SLOT(rangeContentsChanged(KTextEditor::SmartRange*)));
  connect(m_smartRange->primaryNotifier(), SIGNAL(rangeEliminated(KTextEditor::SmartRange*)),             SLOT(rangeEliminated(KTextEditor::SmartRange*)));

  m_smartRange->addWatcher(this);

  KateRegression::self()->addRangeExpectation(this);
}

RangeExpectation::~ RangeExpectation( )
{
  m_smartRange->removeWatcher(this);
  m_smartRange->deletePrimaryNotifier();
}

void RangeExpectation::checkExpectationsFulfilled( ) const
{
  bool fulfilled = true;

  if (m_expectedRange.isValid())
    QCOMPARE(*static_cast<KTextEditor::Range*>(m_smartRange), m_expectedRange);

  for (int i = 0; i < numSignals; ++i) {
    int j = 1 << i;
    int countExpected = (m_expectations & j) ? 1 : 0;

    if (m_notifierNotifications[i] < countExpected)
      { fulfilled = false; kDebug() << "Notifier: Expected to be notified of %1." << nameForSignal(j); }
    else if (m_notifierNotifications[i] > countExpected)
      if (countExpected)
        { fulfilled = false; kDebug() << "Notifier: Notified more than once about %1." << nameForSignal(j); }
      else
        { fulfilled = false; kDebug() << "Notifier: Notified incorrectly about %1." << nameForSignal(j); }

    if (m_watcherNotifications[i] < countExpected)
      { fulfilled = false; kDebug() << "Watcher: Expected to be notified of %1." << nameForSignal(j); }
    else if (m_watcherNotifications[i] > countExpected)
      if (countExpected)
        { fulfilled = false; kDebug() << "Watcher: Notified more than once about %1." << nameForSignal(j); }
      else
        { fulfilled = false; kDebug() << "Watcher: Notified incorrectly about %1." << nameForSignal(j); }
  }
  QVERIFY(fulfilled);
}

void RangeExpectation::signalReceived( int signal )
{
  switch (signal) {
    case PositionChanged:
      signal = 0;
      break;
    case ContentsChanged:
      signal = 1;
      break;
    case Eliminated:
      signal = 2;
      break;
  }

  if (!sender()) {
    Q_ASSERT(!m_watcherNotifications[signal]);
    m_watcherNotifications[signal]++;
  } else {
    Q_ASSERT(!m_notifierNotifications[signal]);
    m_notifierNotifications[signal]++;
  }
}

void RangeExpectation::rangePositionChanged( KTextEditor::SmartRange * range )
{
  QCOMPARE(range, m_smartRange);
  signalReceived(PositionChanged);
}

void RangeExpectation::rangeContentsChanged( KTextEditor::SmartRange * range )
{
  QCOMPARE(range, m_smartRange);
  signalReceived(ContentsChanged);
}

void RangeExpectation::rangeEliminated( KTextEditor::SmartRange * range )
{
  QCOMPARE(range, m_smartRange);
  signalReceived(Eliminated);
}

QString RangeExpectation::nameForSignal( int signal ) const
{
  switch (signal) {
    case PositionChanged:
      return "range position change";
    case ContentsChanged:
      return "range content change";
    case Eliminated:
      return "elimination of the range";
    default:
      return "[invalid signal]";
  }
}

#include "rangeexpectation.moc"
