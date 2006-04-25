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
  //connect(m_smartRange->primaryNotifier(), SIGNAL(boundaryDeleted(KTextEditor::SmartRange*,bool)),   SLOT(boundaryDeleted(KTextEditor::SmartRange*,bool)));
  connect(m_smartRange->primaryNotifier(), SIGNAL(rangeEliminated(KTextEditor::SmartRange*)),             SLOT(rangeEliminated(KTextEditor::SmartRange*)));
  //connect(m_smartRange->primaryNotifier(), SIGNAL(firstCharacterDeleted(KTextEditor::SmartRange*)),  SLOT(firstCharacterDeleted(KTextEditor::SmartRange*)));
  //connect(m_smartRange->primaryNotifier(), SIGNAL(lastCharacterDeleted(KTextEditor::SmartRange*)),   SLOT(lastCharacterDeleted(KTextEditor::SmartRange*)));

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
  for (int i = 0; i < numSignals; ++i) {
    int j = 2 << (i - 1);
    if (m_expectations & j) {
      if (m_notifierNotifications[i] == 0)
        QFAIL(QString("Notifier: Expected to be notified of %1.").arg(nameForSignal(j)).toLatin1());
      else if (m_notifierNotifications[i] > 1)
        QFAIL(QString("Notifier: Notified more than once about %1.").arg(nameForSignal(j)).toLatin1());

      if (m_watcherNotifications[i] == 0)
        QFAIL(QString("Watcher: Expected to be notified of %1.").arg(nameForSignal(j)).toLatin1());
      else if (m_watcherNotifications[i] > 1)
        QFAIL(QString("Watcher: Notified more than once about %1.").arg(nameForSignal(j)).toLatin1());
    }
  }
}

void RangeExpectation::signalReceived( int signal )
{
  QVERIFY(m_expectations & signal);
  QCOMPARE(*static_cast<KTextEditor::Range*>(m_smartRange), m_expectedRange);

  signal = int(log(signal) / log(2));

  if (sender())
    m_watcherNotifications[signal]++;
  else
    m_notifierNotifications[signal]++;
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

/*void RangeExpectation::boundaryDeleted( KTextEditor::SmartRange * range, bool start )
{
  QCOMPARE(range, m_smartRange);

  if (start) {
    signalReceived(StartBoundaryDeleted);

  } else {
    signalReceived(EndBoundaryDeleted);
  }
}*/

void RangeExpectation::rangeEliminated( KTextEditor::SmartRange * range )
{
  QCOMPARE(range, m_smartRange);
  signalReceived(Eliminated);
}

/*void RangeExpectation::firstCharacterDeleted( KTextEditor::SmartRange * range )
{
  QCOMPARE(range, m_smartRange);
  signalReceived(FirstCharacterDeleted);
}

void RangeExpectation::lastCharacterDeleted( KTextEditor::SmartRange * range )
{
  QCOMPARE(range, m_smartRange);
  signalReceived(LastCharacterDeleted);
}*/

QString RangeExpectation::nameForSignal( int signal ) const
{
  switch (signal) {
    case PositionChanged:
      return "position change";
    case ContentsChanged:
      return "content change";
    /*case StartBoundaryDeleted:
      return "starting boundary deletion";
    case EndBoundaryDeleted:
      return "ending boundary deletion";*/
    case Eliminated:
      return "elimination of the range";
    /*case FirstCharacterDeleted:
      return "first character deletion";
    case LastCharacterDeleted:
      return "last character deletion";*/
    default:
      return "[invalid signal]";
  }
}

#include "rangeexpectation.moc"
