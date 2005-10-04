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

#define private public
#define protected public

#include "kateregression.h"

#include "kateglobal.h"
#include "katedocument.h"
#include "katesmartmanager.h"

#include <kaboutdata.h>
#include <kapplication.h>
#include <klocale.h>

#include <QtTest/qttest_kde.h>

QTTEST_KDEMAIN( KateRegression, GUI )

namespace QtTest {
  template<>
  char* toString(const KTextEditor::Cursor& cursor)
  {
    QByteArray ba = "Cursor(";
    ba += QByteArray::number(cursor.line()) + ", " + QByteArray::number(cursor.column());
    ba += ")";
    return qstrdup(ba.data());
  }

  template<>
  char* toString(const KTextEditor::Range& range)
  {
    QByteArray ba = "Range([";
    ba += QByteArray::number(range.start().line()) + ", " + QByteArray::number(range.start().column());
    ba += "] -> [";
    ba += QByteArray::number(range.end().line()) + ", " + QByteArray::number(range.end().column());
    ba += "])";
    return qstrdup(ba.data());
  }
}

// TODO split it various functions
void KateRegression::testAll()
{
  m_doc = dynamic_cast<KateDocument*>(KateGlobal::self()->createDocument(this));
  VERIFY(m_doc);

  // Multi-line insert
  KTextEditor::Cursor* cursor1 = m_doc->newSmartCursor(KTextEditor::Cursor(), false);
  KTextEditor::Cursor* cursor2 = m_doc->newSmartCursor(KTextEditor::Cursor(), true);

  m_doc->insertText(KTextEditor::Cursor(), "Test Text\nMore Test Text");
  COMPARE(m_doc->end(), KTextEditor::Cursor(1,14));

  QString text = m_doc->text(KTextEditor::Range(1,0,1,14));
  COMPARE(text, QString("More Test Text"));

  // Check cursors and ranges have moved properly
  COMPARE(*cursor1, KTextEditor::Cursor(0,0));
  COMPARE(*cursor2, KTextEditor::Cursor(1,14));

  KTextEditor::Cursor cursor3 = m_doc->endOfLine(1);

  // Set up a few more lines
  m_doc->insertText(*cursor2, "\nEven More Test Text");
  COMPARE(m_doc->end(), KTextEditor::Cursor(2,19));
  COMPARE(cursor3, m_doc->endOfLine(1));

  // Intra-line insert
  KTextEditor::Cursor* cursorStartOfLine = m_doc->newSmartCursor(KTextEditor::Cursor(1,0));

  KTextEditor::Cursor* cursorStartOfEdit = m_doc->newSmartCursor(KTextEditor::Cursor(1,5), false);
  KTextEditor::Cursor* cursorEndOfEdit = m_doc->newSmartCursor(KTextEditor::Cursor(1,5), true);

  KTextEditor::Range* rangeEdit = static_cast<KTextEditor::SmartInterface*>(m_doc)->newSmartRange(*cursorStartOfEdit, *cursorEndOfEdit, 0L, KTextEditor::SmartRange::ExpandRight);

  KTextEditor::Cursor* cursorPastEdit = m_doc->newSmartCursor(KTextEditor::Cursor(1,6));
  KTextEditor::Cursor* cursorEOL = m_doc->newSmartCursor(m_doc->endOfLine(1), false);
  KTextEditor::Cursor* cursorEOLMoves = m_doc->newSmartCursor(m_doc->endOfLine(1), true);

  KTextEditor::Cursor* cursorNextLine = m_doc->newSmartCursor(KTextEditor::Cursor(2,0));

  addCursorExpectation(cursorStartOfLine, CursorSignalExpectation(false, false, false, false, false, false));
  addCursorExpectation(cursorStartOfEdit, CursorSignalExpectation(false, false, false, true, false, false));
  addCursorExpectation(cursorEndOfEdit, CursorSignalExpectation(false, false, true, false, true, false));
  addCursorExpectation(cursorPastEdit, CursorSignalExpectation(false, false, false, false, true, false));
  addCursorExpectation(cursorNextLine, CursorSignalExpectation(false, false, false, false, false, false));

  RangeSignalExpectation* rangeEditExpectation = new RangeSignalExpectation(rangeEdit);
  rangeEditExpectation->setExpected(RangeSignalExpectation::signalContentsChanged);
  addRangeExpectation(rangeEditExpectation);

  m_doc->insertText(*cursorStartOfEdit, "Additional ");

  COMPARE(*cursorStartOfLine, KTextEditor::Cursor(1,0));
  COMPARE(*cursorStartOfEdit, KTextEditor::Cursor(1,5));
  COMPARE(*cursorEndOfEdit, KTextEditor::Cursor(1,16));
  COMPARE(*cursorPastEdit, KTextEditor::Cursor(1,17));
  COMPARE(*cursorEOL, m_doc->endOfLine(1));
  COMPARE(*cursorEOLMoves, m_doc->endOfLine(1));
  COMPARE(*cursorNextLine, KTextEditor::Cursor(2,0));

  COMPARE(*rangeEdit, KTextEditor::Range(*cursorStartOfEdit, *cursorEndOfEdit));

  checkSignalExpectations();

  KTextEditor::Cursor* cursorInsideDelete = m_doc->newSmartCursor(KTextEditor::Cursor(1,7));

  addCursorExpectation(cursorStartOfEdit, CursorSignalExpectation(false, true, false, false, false, false));
  addCursorExpectation(cursorInsideDelete, CursorSignalExpectation(false, false, false, false, true, true));
  addCursorExpectation(cursorEndOfEdit, CursorSignalExpectation(true, false, false, false, true, false));
  addCursorExpectation(cursorPastEdit, CursorSignalExpectation(false, false, false, false, true, false));
  addCursorExpectation(cursorNextLine, CursorSignalExpectation(false, false, false, false, false, false));

  // Intra-line remove
  m_doc->removeText(KTextEditor::Range(*cursorStartOfEdit, 11));

  COMPARE(*cursorStartOfLine, KTextEditor::Cursor(1,0));
  COMPARE(*cursorStartOfEdit, KTextEditor::Cursor(1,5));
  COMPARE(*cursorInsideDelete, KTextEditor::Cursor(1,5));
  COMPARE(*cursorEndOfEdit, KTextEditor::Cursor(1,5));
  COMPARE(*cursorPastEdit, KTextEditor::Cursor(1,6));
  COMPARE(*cursorEOL, m_doc->endOfLine(1));
  COMPARE(*cursorEOLMoves, m_doc->endOfLine(1));
  checkSignalExpectations();

  KTextEditor::Cursor oldEOL = *cursorEOL;

  addCursorExpectation(cursorPastEdit, CursorSignalExpectation(false, false, false, false, false, false));
  addCursorExpectation(cursorEOL, CursorSignalExpectation(false, false, false, true, false, false));
  addCursorExpectation(cursorEOLMoves, CursorSignalExpectation(false, false, true, false, true, false));
  addCursorExpectation(cursorNextLine, CursorSignalExpectation(false, false, false, false, false, false));

  // Insert at EOL
  m_doc->insertText(m_doc->endOfLine(1), " Even More");

  COMPARE(*cursorEOL, oldEOL);
  COMPARE(*cursorEOLMoves, m_doc->endOfLine(1));
  checkSignalExpectations();

  *cursorEOL = *cursorEOLMoves;

  addCursorExpectation(cursorPastEdit, CursorSignalExpectation(false, false, false, false, false, false));
  addCursorExpectation(cursorEOL, CursorSignalExpectation(false, false, false, true, false, false));
  addCursorExpectation(cursorEOLMoves, CursorSignalExpectation(false, false, true, false, true, false));
  addCursorExpectation(cursorNextLine, CursorSignalExpectation(false, false, false, false, true, false));

  // wrap line
  m_doc->insertText(m_doc->endOfLine(1), "\n");

  COMPARE(*cursorEOL,m_doc->endOfLine(1));
  COMPARE(*cursorEOLMoves, KTextEditor::Cursor(2, 0));
  checkSignalExpectations();

  // Remove line wrapping
  m_doc->removeText(KTextEditor::Range(m_doc->endOfLine(1), KTextEditor::Cursor(2, 0)));

  COMPARE(*cursorEOL,m_doc->endOfLine(1));
  COMPARE(*cursorEOLMoves, m_doc->endOfLine(1));

  checkSmartManager();
  //m_doc->smartManager()->debugOutput();
}

void KateRegression::checkSmartManager()
{
  KateSmartGroup* currentGroup = m_doc->smartManager()->groupForLine(0);
  VERIFY(currentGroup);

  forever {
    if (!currentGroup->previous())
      COMPARE(currentGroup->startLine(), 0);

    foreach (KateSmartCursor* cursor, currentGroup->feedbackCursors()) {
      VERIFY(currentGroup->containsLine(cursor->line()));
      COMPARE(cursor->m_smartGroup, currentGroup);
    }

    if (!currentGroup->next())
      break;

    COMPARE(currentGroup->endLine(), currentGroup->next()->startLine() - 1);
    COMPARE(currentGroup->next()->previous(), currentGroup);
  }

  COMPARE(currentGroup->endLine(), m_doc->lines() - 1);
}

CursorSignalExpectation::CursorSignalExpectation( bool a, bool b, bool c, bool d, bool e, bool f)
  : notifierExpectCharacterDeletedBefore(a)
  , notifierExpectCharacterDeletedAfter(b)
  , notifierExpectCharacterInsertedBefore(c)
  , notifierExpectCharacterInsertedAfter(d)
  , notifierExpectPositionChanged(e)
  , notifierExpectPositionDeleted(f)
  , watcherExpectCharacterDeletedBefore(a)
  , watcherExpectCharacterDeletedAfter(b)
  , watcherExpectCharacterInsertedBefore(c)
  , watcherExpectCharacterInsertedAfter(d)
  , watcherExpectPositionChanged(e)
  , watcherExpectPositionDeleted(f)
  , notifierCharacterDeletedBefore(0)
  , notifierCharacterDeletedAfter(0)
  , notifierCharacterInsertedBefore(0)
  , notifierCharacterInsertedAfter(0)
  , notifierPositionChanged(0)
  , notifierPositionDeleted(0)
  , watcherCharacterDeletedBefore(0)
  , watcherCharacterDeletedAfter(0)
  , watcherCharacterInsertedBefore(0)
  , watcherCharacterInsertedAfter(0)
  , watcherPositionChanged(0)
  , watcherPositionDeleted(0)
{
}

void KateRegression::addCursorExpectation(KTextEditor::Cursor* c, const CursorSignalExpectation & expectation )
{
  KTextEditor::SmartCursor* cursor = static_cast<KTextEditor::SmartCursor*>(c);
  connect(cursor->notifier(), SIGNAL(characterDeleted(KTextEditor::SmartCursor*, bool)), this, SLOT(slotCharacterDeleted(KTextEditor::SmartCursor*, bool)));
  connect(cursor->notifier(), SIGNAL(characterInserted(KTextEditor::SmartCursor*, bool)), this, SLOT(slotCharacterInserted(KTextEditor::SmartCursor*, bool)));
  connect(cursor->notifier(), SIGNAL(positionChanged(KTextEditor::SmartCursor*)), this, SLOT(slotPositionChanged(KTextEditor::SmartCursor*)));
  connect(cursor->notifier(), SIGNAL(positionDeleted(KTextEditor::SmartCursor*)), this, SLOT(slotPositionDeleted(KTextEditor::SmartCursor*)));
  cursor->setWatcher(this);
  m_cursorExpectations.insert(cursor, expectation);
}

void KateRegression::slotCharacterDeleted( KTextEditor::SmartCursor * cursor, bool before )
{
  VERIFY(m_cursorExpectations.contains(cursor));

  if (before) {
    VERIFY(m_cursorExpectations[cursor].notifierExpectCharacterDeletedBefore);
    m_cursorExpectations[cursor].notifierCharacterDeletedBefore++;

  } else {
    VERIFY(m_cursorExpectations[cursor].notifierExpectCharacterDeletedAfter);
    m_cursorExpectations[cursor].notifierCharacterDeletedAfter++;
  }
}

void KateRegression::characterDeleted( KTextEditor::SmartCursor * cursor, bool deletedBefore )
{
  VERIFY(m_cursorExpectations.contains(cursor));

  if (deletedBefore) {
    VERIFY(m_cursorExpectations[cursor].watcherExpectCharacterDeletedBefore);
    m_cursorExpectations[cursor].watcherCharacterDeletedBefore++;

  } else {
    VERIFY(m_cursorExpectations[cursor].watcherExpectCharacterDeletedAfter);
    m_cursorExpectations[cursor].watcherCharacterDeletedAfter++;
  }
}

void KateRegression::slotCharacterInserted( KTextEditor::SmartCursor * cursor, bool before )
{
  VERIFY(m_cursorExpectations.contains(cursor));

  if (before) {
    VERIFY(m_cursorExpectations[cursor].notifierExpectCharacterInsertedBefore);
    m_cursorExpectations[cursor].notifierCharacterInsertedBefore++;

  } else {
    VERIFY(m_cursorExpectations[cursor].notifierExpectCharacterInsertedAfter);
    m_cursorExpectations[cursor].notifierCharacterInsertedAfter++;
  }
}

void KateRegression::characterInserted( KTextEditor::SmartCursor * cursor, bool insertedBefore )
{
  VERIFY(m_cursorExpectations.contains(cursor));

  if (insertedBefore) {
    VERIFY(m_cursorExpectations[cursor].watcherExpectCharacterInsertedBefore);
    m_cursorExpectations[cursor].watcherCharacterInsertedBefore++;

  } else {
    VERIFY(m_cursorExpectations[cursor].watcherExpectCharacterInsertedAfter);
    m_cursorExpectations[cursor].watcherCharacterInsertedAfter++;
  }
}

void KateRegression::positionChanged( KTextEditor::SmartCursor * cursor )
{
  VERIFY(m_cursorExpectations.contains(cursor));
  VERIFY(m_cursorExpectations[cursor].watcherExpectPositionChanged);
  m_cursorExpectations[cursor].watcherPositionChanged++;
}

void KateRegression::slotPositionChanged( KTextEditor::SmartCursor * cursor )
{
  VERIFY(m_cursorExpectations.contains(cursor));
  if (!m_cursorExpectations[cursor].notifierExpectPositionChanged)
    VERIFY(m_cursorExpectations[cursor].notifierExpectPositionChanged);
  m_cursorExpectations[cursor].notifierPositionChanged++;
}

void KateRegression::slotPositionDeleted( KTextEditor::SmartCursor * cursor )
{
  VERIFY(m_cursorExpectations.contains(cursor));
  VERIFY(m_cursorExpectations[cursor].notifierExpectPositionDeleted);
  m_cursorExpectations[cursor].notifierPositionDeleted++;
}

void KateRegression::positionDeleted( KTextEditor::SmartCursor * cursor )
{
  VERIFY(m_cursorExpectations.contains(cursor));
  VERIFY(m_cursorExpectations[cursor].watcherExpectPositionDeleted);
  m_cursorExpectations[cursor].watcherPositionDeleted++;
}

void KateRegression::checkSignalExpectations( )
{
  QMapIterator<KTextEditor::SmartCursor*, CursorSignalExpectation> it = m_cursorExpectations;
  while (it.hasNext()) {
    it.next();
    it.value().checkExpectationsFulfilled();
    it.key()->setWatcher(0L);
    it.key()->deleteNotifier();
  }
  m_cursorExpectations.clear();

  foreach (RangeSignalExpectation* e, m_rangeExpectations)
    e->checkExpectationsFulfilled();
  qDeleteAll(m_rangeExpectations);
  m_rangeExpectations.clear();
}

void CursorSignalExpectation::checkExpectationsFulfilled( ) const
{
  if (notifierExpectCharacterDeletedBefore && notifierCharacterDeletedBefore == 0)
    FAIL("Notifier: Expected to be notified of a character to be deleted before cursor.");
  if (notifierExpectCharacterDeletedAfter && notifierCharacterDeletedAfter == 0)
    FAIL("Notifier: Expected to be notified of a character to be deleted after cursor.");
  if (notifierExpectCharacterInsertedBefore && notifierCharacterInsertedBefore == 0)
    FAIL("Notifier: Expected to be notified of a character to be inserted before cursor.");
  if (notifierExpectCharacterInsertedAfter && notifierCharacterInsertedAfter == 0)
    FAIL("Notifier: Expected to be notified of a character to be inserted after cursor.");
  if (notifierExpectPositionChanged && notifierPositionChanged == 0)
    FAIL("Notifier: Expected to be notified of the cursor's position change.");
  if (notifierExpectPositionDeleted && notifierPositionDeleted == 0)
    FAIL("Notifier: Expected to be notified of the cursor's position deletion.");

  if (notifierCharacterDeletedBefore > 1)
    FAIL("Notifier: Notified more than once about a character to be deleted before cursor.");
  if (notifierCharacterDeletedAfter > 1)
    FAIL("Notifier: Notified more than once about a character to be deleted after cursor.");
  if (notifierCharacterInsertedBefore > 1)
    FAIL("Notifier: Notified more than once about a character to be inserted before cursor.");
  if (notifierCharacterInsertedAfter > 1)
    FAIL("Notifier: Notified more than once about a character to be inserted after cursor.");
  if (notifierPositionChanged > 1)
    FAIL("Notifier: Notified more than once about the cursor's position change.");
  if (notifierPositionDeleted > 1)
    FAIL("Notifier: Notified more than once about the cursor's position deletion.");

  if (watcherExpectCharacterDeletedBefore && watcherCharacterDeletedBefore == 0)
    FAIL("Watcher: Expected to be notified of a character to be deleted before cursor.");
  if (watcherExpectCharacterDeletedAfter && watcherCharacterDeletedAfter == 0)
    FAIL("Watcher: Expected to be notified of a character to be deleted after cursor.");
  if (watcherExpectCharacterInsertedBefore && watcherCharacterInsertedBefore == 0)
    FAIL("Watcher: Expected to be notified of a character to be inserted before cursor.");
  if (watcherExpectCharacterInsertedAfter && watcherCharacterInsertedAfter == 0)
    FAIL("Watcher: Expected to be notified of a character to be inserted after cursor.");
  if (watcherExpectPositionChanged && watcherPositionChanged == 0)
    FAIL("Watcher: Expected to be notified of the cursor's position change.");
  if (watcherExpectPositionDeleted && watcherPositionDeleted == 0)
    FAIL("Watcher: Expected to be notified of the cursor's position deletion.");

  if (watcherCharacterDeletedBefore > 1)
    FAIL("Watcher: Notified more than once about a character to be deleted before cursor.");
  if (watcherCharacterDeletedAfter > 1)
    FAIL("Watcher: Notified more than once about a character to be deleted after cursor.");
  if (watcherCharacterInsertedBefore > 1)
    FAIL("Watcher: Notified more than once about a character to be inserted before cursor.");
  if (watcherCharacterInsertedAfter > 1)
    FAIL("Watcher: Notified more than once about a character to be inserted after cursor.");
  if (watcherPositionChanged > 1)
    FAIL("Watcher: Notified more than once about the cursor's position change.");
  if (watcherPositionDeleted > 1)
    FAIL("Watcher: Notified more than once about the cursor's position deletion.");
}

RangeSignalExpectation::RangeSignalExpectation(KTextEditor::Range* range)
  : smartRange(dynamic_cast<KTextEditor::SmartRange*>(range))
{
  Q_ASSERT(smartRange);
  for (int i = 0; i < numSignals; ++i) {
    expectations[i] = false;
    notifierNotifications[i] = 0;
    watcherNotifications[i] = 0;
  }
  smartRange->setWatcher(this);
  connect(smartRange->notifier(), SIGNAL(positionChanged(KTextEditor::SmartRange*)),        SLOT(positionChanged(KTextEditor::SmartRange*)));
  connect(smartRange->notifier(), SIGNAL(contentsChanged(KTextEditor::SmartRange*)),        SLOT(contentsChanged(KTextEditor::SmartRange*)));
  connect(smartRange->notifier(), SIGNAL(boundaryDeleted(KTextEditor::SmartRange*,bool)),   SLOT(boundaryDeleted(KTextEditor::SmartRange*,bool)));
  connect(smartRange->notifier(), SIGNAL(eliminated(KTextEditor::SmartRange*)),             SLOT(eliminated(KTextEditor::SmartRange*)));
  connect(smartRange->notifier(), SIGNAL(firstCharacterDeleted(KTextEditor::SmartRange*)),  SLOT(firstCharacterDeleted(KTextEditor::SmartRange*)));
  connect(smartRange->notifier(), SIGNAL(lastCharacterDeleted(KTextEditor::SmartRange*)),   SLOT(lastCharacterDeleted(KTextEditor::SmartRange*)));
}

void RangeSignalExpectation::checkExpectationsFulfilled( ) const
{
  for (int i = 0; i < numSignals; ++i) {
    if (expectations[i]) {
      if (notifierNotifications[i] == 0)
        FAIL(QString("Notifier: Expected to be notified of %1.").arg(nameForSignal(i)).toLatin1());
      else if (notifierNotifications[i] > 1)
        FAIL(QString("Notifier: Notified more than once about %1.").arg(nameForSignal(i)).toLatin1());

      if (watcherNotifications[i] == 0)
        FAIL(QString("Watcher: Expected to be notified of %1.").arg(nameForSignal(i)).toLatin1());
      else if (watcherNotifications[i] > 1)
        FAIL(QString("Watcher: Notified more than once about %1.").arg(nameForSignal(i)).toLatin1());
    }
  }
}

void RangeSignalExpectation::setExpected( int signal )
{
  Q_ASSERT(signal >= 0 && signal < numSignals);
  expectations[signal] = true;
}

void RangeSignalExpectation::positionChanged( KTextEditor::SmartRange * )
{
  VERIFY(expectations[signalPositionChanged]);

  if (sender())
    watcherNotifications[signalPositionChanged]++;
  else
    notifierNotifications[signalPositionChanged]++;
}

void RangeSignalExpectation::contentsChanged( KTextEditor::SmartRange * )
{
  VERIFY(expectations[signalContentsChanged]);

  if (sender())
    watcherNotifications[signalContentsChanged]++;
  else
    notifierNotifications[signalContentsChanged]++;
}

void RangeSignalExpectation::boundaryDeleted( KTextEditor::SmartRange * , bool start )
{
  if (start) {
    VERIFY(expectations[signalStartBoundaryDeleted]);
    if (sender())
      watcherNotifications[signalStartBoundaryDeleted]++;
    else
      notifierNotifications[signalStartBoundaryDeleted]++;

  } else {
    VERIFY(expectations[signalEndBoundaryDeleted]);
    if (sender())
      watcherNotifications[signalEndBoundaryDeleted]++;
    else
      notifierNotifications[signalEndBoundaryDeleted]++;
  }
}

void RangeSignalExpectation::eliminated( KTextEditor::SmartRange * )
{
  VERIFY(expectations[signalEliminated]);

  if (sender())
    watcherNotifications[signalEliminated]++;
  else
    notifierNotifications[signalEliminated]++;
}

void RangeSignalExpectation::firstCharacterDeleted( KTextEditor::SmartRange * )
{
  VERIFY(expectations[signalFirstCharacterDeleted]);

  if (sender())
    watcherNotifications[signalFirstCharacterDeleted]++;
  else
    notifierNotifications[signalFirstCharacterDeleted]++;
}

void RangeSignalExpectation::lastCharacterDeleted( KTextEditor::SmartRange * )
{
  VERIFY(expectations[signalLastCharacterDeleted]);

  if (sender())
    watcherNotifications[signalLastCharacterDeleted]++;
  else
    notifierNotifications[signalLastCharacterDeleted]++;
}

RangeSignalExpectation::~ RangeSignalExpectation( )
{
  smartRange->setWatcher(0L);
  smartRange->deleteNotifier();
}

void KateRegression::addRangeExpectation( RangeSignalExpectation * expectation )
{
  m_rangeExpectations.append(expectation);
}

QString RangeSignalExpectation::nameForSignal( int signal ) const
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

#include "kateregression.moc"
