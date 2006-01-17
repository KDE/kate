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

#ifndef CURSOREXPECTATION_H
#define CURSOREXPECTATION_H

#include <QFlags>
#include <ktexteditor/cursor.h>
#include <ktexteditor/cursorfeedback.h>

class CursorExpectation : public QObject, public KTextEditor::SmartCursorWatcher
{
  Q_OBJECT

  public:
    enum CursorSignal {
      NoSignal = 0x0,
      CharacterDeletedBefore = 0x1,
      CharacterDeletedAfter = 0x2,
      CharacterInsertedBefore = 0x4,
      CharacterInsertedAfter = 0x8,
      PositionChanged = 0x10,
      PositionDeleted = 0x20
    };
    static const int numSignals = 6;
    Q_DECLARE_FLAGS(CursorSignals, CursorSignal)

    CursorExpectation(KTextEditor::Cursor* cursor, CursorSignals signalsExpected = NoSignal, const KTextEditor::Cursor& positionExpected = KTextEditor::Cursor::invalid());
    virtual ~CursorExpectation();

    void checkExpectationsFulfilled() const;

  public Q_SLOTS:
    virtual void positionChanged(KTextEditor::SmartCursor* cursor);
    virtual void positionDeleted(KTextEditor::SmartCursor* cursor);
    virtual void characterDeleted(KTextEditor::SmartCursor* cursor, bool deletedBefore);
    virtual void characterInserted(KTextEditor::SmartCursor* cursor, bool insertedBefore);

  private:
    QString nameForSignal(int signal) const;
    void signalReceived(int signal);

    KTextEditor::SmartCursor* m_smartCursor;
    KTextEditor::Cursor m_expectedCursor;

    CursorSignals m_expectations;

    int m_notifierNotifications[numSignals];
    int m_watcherNotifications[numSignals];
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CursorExpectation::CursorSignals)

#endif
