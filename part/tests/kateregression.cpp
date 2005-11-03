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

#include "kateregression.h"

#include <ktexteditor/editorchooser.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/document.h>
#include <ktexteditor/smartrange.h>
#include <ktexteditor/smartinterface.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <klocale.h>

#include <QtTest/qttest_kde.h>

#include "cursorexpectation.h"
#include "rangeexpectation.h"

QTTEST_KDEMAIN( KateRegression, GUI )

using namespace KTextEditor;

KateRegression* KateRegression::s_self = 0L;

namespace QtTest {
  template<>
  char* toString(const Cursor& cursor)
  {
    QByteArray ba = "Cursor(";
    ba += QByteArray::number(cursor.line()) + ", " + QByteArray::number(cursor.column());
    ba += ")";
    return qstrdup(ba.data());
  }

  template<>
  char* toString(const Range& range)
  {
    QByteArray ba = "Range[(";
    ba += QByteArray::number(range.start().line()) + ", " + QByteArray::number(range.start().column());
    ba += ") -> (";
    ba += QByteArray::number(range.end().line()) + ", " + QByteArray::number(range.end().column());
    ba += ")]";
    return qstrdup(ba.data());
  }
}

// TODO split it various functions
void KateRegression::testAll()
{
  Editor* editor = EditorChooser::editor();
  VERIFY(editor);

  m_doc = editor->createDocument(this);
  VERIFY(m_doc);

  VERIFY(smart());

  // Multi-line insert
  Cursor* cursor1 = smart()->newSmartCursor(Cursor(), false);
  Cursor* cursor2 = smart()->newSmartCursor(Cursor(), true);

  m_doc->insertText(Cursor(), "Test Text\nMore Test Text");
  COMPARE(m_doc->documentEnd(), Cursor(1,14));

  QString text = m_doc->text(Range(1,0,1,14));
  COMPARE(text, QString("More Test Text"));

  // Check cursors and ranges have moved properly
  COMPARE(*cursor1, Cursor(0,0));
  COMPARE(*cursor2, Cursor(1,14));

  Cursor cursor3 = m_doc->endOfLine(1);

  // Set up a few more lines
  m_doc->insertText(*cursor2, "\nEven More Test Text");
  COMPARE(m_doc->documentEnd(), Cursor(2,19));
  COMPARE(cursor3, m_doc->endOfLine(1));

  // Intra-line insert
  Cursor* cursorStartOfLine = smart()->newSmartCursor(Cursor(1,0));

  Cursor* cursorStartOfEdit = smart()->newSmartCursor(Cursor(1,5), false);
  Cursor* cursorEndOfEdit = smart()->newSmartCursor(Cursor(1,5), true);

  Range* rangeEdit = smart()->newSmartRange(*cursorStartOfEdit, *cursorEndOfEdit, 0L, SmartRange::ExpandRight);

  Cursor* cursorPastEdit = smart()->newSmartCursor(Cursor(1,6));
  Cursor* cursorEOL = smart()->newSmartCursor(m_doc->endOfLine(1), false);
  Cursor* cursorEOLMoves = smart()->newSmartCursor(m_doc->endOfLine(1), true);

  Cursor* cursorNextLine = smart()->newSmartCursor(Cursor(2,0));

  new CursorExpectation(cursorStartOfLine);
  new CursorExpectation(cursorStartOfEdit, CursorExpectation::CharacterInsertedAfter);
  new CursorExpectation(cursorEndOfEdit, CursorExpectation::CharacterInsertedBefore | CursorExpectation::PositionChanged, Cursor(1,16));
  new CursorExpectation(cursorPastEdit, CursorExpectation::PositionChanged, Cursor(1,17));
  new CursorExpectation(cursorNextLine);

  new RangeExpectation(rangeEdit, RangeExpectation::ContentsChanged, Range(*cursorStartOfEdit, Cursor(1,16)));

  m_doc->insertText(*cursorStartOfEdit, "Additional ");

  COMPARE(*cursorEOL, m_doc->endOfLine(1));
  COMPARE(*cursorEOLMoves, m_doc->endOfLine(1));

  checkSignalExpectations();

  Cursor* cursorInsideDelete = smart()->newSmartCursor(Cursor(1,7));

  new CursorExpectation(cursorStartOfEdit, CursorExpectation::CharacterDeletedAfter);
  new CursorExpectation(cursorInsideDelete, CursorExpectation::PositionChanged | CursorExpectation::PositionDeleted, *cursorStartOfEdit);
  new CursorExpectation(cursorEndOfEdit, CursorExpectation::CharacterDeletedBefore | CursorExpectation::PositionChanged, *cursorStartOfEdit);
  new CursorExpectation(cursorPastEdit, CursorExpectation::PositionChanged, Cursor(1,6));
  new CursorExpectation(cursorNextLine, CursorExpectation::NoSignal, Cursor(2,0));

  new RangeExpectation(rangeEdit, RangeExpectation::ContentsChanged, Range(*cursorStartOfEdit, *cursorStartOfEdit));

  // Intra-line remove
  m_doc->removeText(Range(*cursorStartOfEdit, 11));

  COMPARE(*cursorEOL, m_doc->endOfLine(1));
  COMPARE(*cursorEOLMoves, m_doc->endOfLine(1));
  checkSignalExpectations();

  Cursor oldEOL = *cursorEOL;

  new CursorExpectation(cursorPastEdit);
  new CursorExpectation(cursorEOL, CursorExpectation::CharacterInsertedAfter);
  new CursorExpectation(cursorEOLMoves, CursorExpectation::CharacterInsertedBefore | CursorExpectation::PositionChanged, *cursorEOL + Cursor(0,10));
  new CursorExpectation(cursorNextLine);

  // Insert at EOL
  m_doc->insertText(m_doc->endOfLine(1), " Even More");

  checkSignalExpectations();

  *cursorEOL = *cursorEOLMoves;

  new CursorExpectation(cursorPastEdit);
  new CursorExpectation(cursorEOL, CursorExpectation::CharacterInsertedAfter);
  new CursorExpectation(cursorEOLMoves, CursorExpectation::CharacterInsertedBefore | CursorExpectation::PositionChanged, Cursor(2, 0));
  new CursorExpectation(cursorNextLine, CursorExpectation::PositionChanged, Cursor(3, 0));

  // wrap line
  m_doc->insertText(m_doc->endOfLine(1), "\n");

  checkSignalExpectations();

  // Remove line wrapping
  m_doc->removeText(Range(m_doc->endOfLine(1), Cursor(2, 0)));

  COMPARE(*cursorEOL,m_doc->endOfLine(1));
  COMPARE(*cursorEOLMoves, m_doc->endOfLine(1));
}

void KateRegression::checkSignalExpectations( )
{
  foreach (CursorExpectation* e, m_cursorExpectations)
    e->checkExpectationsFulfilled();
  qDeleteAll(m_cursorExpectations);
  m_cursorExpectations.clear();

  foreach (RangeExpectation* e, m_rangeExpectations)
    e->checkExpectationsFulfilled();
  qDeleteAll(m_rangeExpectations);
  m_rangeExpectations.clear();
}

void KateRegression::addRangeExpectation( RangeExpectation * expectation )
{
  m_rangeExpectations.append(expectation);
}

void KateRegression::addCursorExpectation( CursorExpectation * expectation )
{
  m_cursorExpectations.append(expectation);
}

KateRegression::KateRegression( )
{
  s_self = this;
}

KateRegression * KateRegression::self( )
{
  return s_self;
}

void KateRegression::testRange( )
{
  Range r;
  checkRange(r);
}

void KateRegression::testSmartRange( )
{
  SmartRange* range = smart()->newSmartRange();
  checkRange(*range);
  delete range;
}

void KateRegression::checkRange( Range & valid )
{
  VERIFY(valid.isValid() && valid.start() <= valid.end());

  Cursor before(0,1), start(0,2), end(1,4), after(1,10);

  Range result(start, end);
  VERIFY(valid.isValid() && valid.start() <= valid.end());

  valid.setRange(start, end);
  VERIFY(valid.isValid() && valid.start() <= valid.end());
  COMPARE(valid, result);

  valid.setRange(end, start);
  VERIFY(valid.isValid() && valid.start() <= valid.end());
  COMPARE(valid, result);

  valid.start() = after;
  VERIFY(valid.isValid() && valid.start() <= valid.end());
  COMPARE(valid, Range(after, after));

  valid = result;
  COMPARE(valid, result);

  valid.end() = before;
  VERIFY(valid.isValid() && valid.start() <= valid.end());
  COMPARE(valid, Range(before, before));
}

void KateRegression::testRangeTree( )
{
  SmartRange* top = smart()->newSmartRange(m_doc->documentRange());

  Range second(1, 2, 1, 10);
  Range* secondLevel = smart()->newSmartRange(second, top);
  COMPARE(*secondLevel, second);

  // Check creation restriction
  Range third(1, 1, 1, 11);
  Range* thirdLevel = smart()->newSmartRange(third, static_cast<SmartRange*>(secondLevel));
  COMPARE(*thirdLevel, third);
  COMPARE(*secondLevel, third);

  Range fourth(1, 4, 1, 6);
  Range* fourthLevel = smart()->newSmartRange(fourth, static_cast<SmartRange*>(thirdLevel));
  COMPARE(*fourthLevel, fourth);

  Range fourth2(1, 7, 1, 8);
  Range* fourth2Level = smart()->newSmartRange(fourth2, static_cast<SmartRange*>(thirdLevel));
  COMPARE(*fourthLevel, fourth);
  COMPARE(*fourth2Level, fourth2);

  // Check moving start before parent
  thirdLevel->start().setColumn(1);
  COMPARE(thirdLevel->start(), Cursor(1,1));
  COMPARE(*thirdLevel, *secondLevel);

  // Check moving end after parent
  thirdLevel->end().setColumn(11);
  COMPARE(thirdLevel->end(), Cursor(1,11));
  COMPARE(*thirdLevel, *secondLevel);

  // Check moving parent after child start
  secondLevel->start() = second.start();
  COMPARE(secondLevel->start(), second.start());
  COMPARE(*thirdLevel, *secondLevel);

  // Check moving parent before child end
  secondLevel->end() = second.end();
  COMPARE(secondLevel->end(), second.end());
  COMPARE(*thirdLevel, *secondLevel);

  top->deleteChildRanges();

  VERIFY(!top->childRanges().count());

  // Test out-of-order creation
  Range range1(Cursor(1,2),2);
  Range range2(Cursor(1,6),3);
  Range range3(Cursor(1,5),1);
  SmartRange* child1 = smart()->newSmartRange(range1, static_cast<SmartRange*>(top));
  Range* childR1 = child1;

  SmartRange* child2 = smart()->newSmartRange(range2, static_cast<SmartRange*>(top));
  Range* childR2 = child2;

  SmartRange* child3 = smart()->newSmartRange(range3, static_cast<SmartRange*>(top));
  Range* childR3 = child3;

  QList<SmartRange*> childList;
  childList << child1 << child3 << child2;

  COMPARE(childList, top->childRanges());
  COMPARE(*childR1, range1);
  COMPARE(*childR2, range2);
  COMPARE(*childR3, range3);

  // Test moving child ranges
  range3 = Range(Cursor(1,5),3);
  *child3 = range3;

  range2.start() = range3.end();

  COMPARE(childList, top->childRanges());
  COMPARE(*childR1, range1);
  COMPARE(*childR2, range2);
  COMPARE(*childR3, range3);

  top->deleteChildRanges();
  delete top;
}

SmartInterface * KateRegression::smart( ) const
{
  return dynamic_cast<SmartInterface*>(const_cast<Document*>(m_doc));
}

#include "kateregression.moc"
