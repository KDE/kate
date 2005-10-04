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

#ifndef KATEREGRESSION_H
#define KATEREGRESSION_H

#include <QObject>
#include <QMap>

#include <ktexteditor/cursor.h>
#include <ktexteditor/range.h>

class KateDocument;

struct CursorSignalExpectation
{
  CursorSignalExpectation(bool delBefore = false, bool delAfter = false, bool insBefore = false, bool insAfter = false, bool posChanged = false, bool posDeleted = false);
  void checkExpectationsFulfilled() const;

  bool notifierExpectCharacterDeletedBefore;
  bool notifierExpectCharacterDeletedAfter;
  bool notifierExpectCharacterInsertedBefore;
  bool notifierExpectCharacterInsertedAfter;
  bool notifierExpectPositionChanged;
  bool notifierExpectPositionDeleted;

  bool watcherExpectCharacterDeletedBefore;
  bool watcherExpectCharacterDeletedAfter;
  bool watcherExpectCharacterInsertedBefore;
  bool watcherExpectCharacterInsertedAfter;
  bool watcherExpectPositionChanged;
  bool watcherExpectPositionDeleted;

  int notifierCharacterDeletedBefore;
  int notifierCharacterDeletedAfter;
  int notifierCharacterInsertedBefore;
  int notifierCharacterInsertedAfter;
  int notifierPositionChanged;
  int notifierPositionDeleted;

  int watcherCharacterDeletedBefore;
  int watcherCharacterDeletedAfter;
  int watcherCharacterInsertedBefore;
  int watcherCharacterInsertedAfter;
  int watcherPositionChanged;
  int watcherPositionDeleted;
};

class RangeSignalExpectation : public QObject, public KTextEditor::SmartRangeWatcher
{
  Q_OBJECT

  public:
    RangeSignalExpectation(KTextEditor::Range* range);
    virtual ~RangeSignalExpectation();

    enum signal {
      signalPositionChanged = 0,
      signalContentsChanged,
      signalStartBoundaryDeleted,
      signalEndBoundaryDeleted,
      signalEliminated,
      signalFirstCharacterDeleted,
      signalLastCharacterDeleted,
      numSignals
    };

    void checkExpectationsFulfilled() const;
    void setExpected(int signal);

  public slots:
    virtual void positionChanged(KTextEditor::SmartRange* range);
    virtual void contentsChanged(KTextEditor::SmartRange* range);
    virtual void boundaryDeleted(KTextEditor::SmartRange* range, bool start);
    virtual void eliminated(KTextEditor::SmartRange* range);
    virtual void firstCharacterDeleted(KTextEditor::SmartRange* range);
    virtual void lastCharacterDeleted(KTextEditor::SmartRange* range);

  private:
    QString nameForSignal(int signal) const;

    KTextEditor::SmartRange* smartRange;

    bool expectations[numSignals];

    int notifierNotifications[numSignals];
    int watcherNotifications[numSignals];
};

class KateRegression : public QObject, public KTextEditor::SmartCursorWatcher
{
  Q_OBJECT

  public:
    virtual void positionChanged(KTextEditor::SmartCursor* cursor);
    virtual void positionDeleted(KTextEditor::SmartCursor* cursor);
    virtual void characterDeleted(KTextEditor::SmartCursor* cursor, bool deletedBefore);
    virtual void characterInserted(KTextEditor::SmartCursor* cursor, bool insertedBefore);

  public slots:
    void slotCharacterDeleted(KTextEditor::SmartCursor* cursor, bool before);
    void slotCharacterInserted(KTextEditor::SmartCursor* cursor, bool before);
    void slotPositionChanged(KTextEditor::SmartCursor* cursor);
    void slotPositionDeleted(KTextEditor::SmartCursor* cursor);

  private slots:
    void testAll();

  private:
    void checkSmartManager();
    void addCursorExpectation(KTextEditor::Cursor* cursor, const CursorSignalExpectation& expectation);
    void addRangeExpectation(RangeSignalExpectation* expectation);
    void checkSignalExpectations();

    KateDocument* m_doc;
    QMap<KTextEditor::SmartCursor*, CursorSignalExpectation> m_cursorExpectations;
    QList<RangeSignalExpectation*> m_rangeExpectations;
};

#endif
