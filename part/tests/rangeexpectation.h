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

#ifndef RANGEEXPECTATION_H
#define RANGEEXPECTATION_H

#include <QFlags>

#include <ktexteditor/range.h>
#include <ktexteditor/rangefeedback.h>

class RangeExpectation : public QObject, public KTextEditor::SmartRangeWatcher
{
  Q_OBJECT

  public:
    enum RangeSignal {
      NoSignal = 0x0,
      PositionChanged = 0x1,
      ContentsChanged = 0x2,
      Eliminated = 0x4
    };
    static const int numSignals = 3;
    Q_DECLARE_FLAGS(RangeSignals, RangeSignal)

    RangeExpectation(KTextEditor::Range* range, RangeSignals signalsExpected = NoSignal, const KTextEditor::Range& rangeExpected = KTextEditor::Range::invalid());
    virtual ~RangeExpectation();

    void checkExpectationsFulfilled() const;

  public Q_SLOTS:
    virtual void positionChanged(KTextEditor::SmartRange* range);
    virtual void contentsChanged(KTextEditor::SmartRange* range);
    //virtual void boundaryDeleted(KTextEditor::SmartRange* range, bool start);
    virtual void eliminated(KTextEditor::SmartRange* range);
    //virtual void firstCharacterDeleted(KTextEditor::SmartRange* range);
    //virtual void lastCharacterDeleted(KTextEditor::SmartRange* range);

  private:
    QString nameForSignal(int signal) const;
    void signalReceived(int signal);

    KTextEditor::SmartRange* m_smartRange;
    KTextEditor::Range m_expectedRange;

    RangeSignals m_expectations;

    int m_notifierNotifications[numSignals];
    int m_watcherNotifications[numSignals];
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RangeExpectation::RangeSignals)

#endif
