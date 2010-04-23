/* This file is part of the KDE libraries
   Copyright (C) 2010 Dominik Haumann <dhaumann kde org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "movingcursor_test.h"
#include "moc_movingcursor_test.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <kateview.h>
#include <kateconfig.h>
#include <ktexteditor/movingcursor.h>

QTEST_KDEMAIN(MovingCursorTest, GUI)

// namespace QTest {
//   template<>
//   char *toString(const KTextEditor::Range &range)
//   {
//     QByteArray ba = "Range[";
//     ba += QByteArray::number(range.start().line()) + ", " + QByteArray::number(range.start().column()) + ", ";
//     ba += QByteArray::number(range.end().line())   + ", " + QByteArray::number(range.end().column());
//     ba += "]";
//     return qstrdup(ba.data());
//   }
// }

MovingCursorTest::MovingCursorTest()
  : QObject()
{
}

MovingCursorTest::~MovingCursorTest()
{
}

void MovingCursorTest::testMovingCursor()
{
}

// tests:
// - atStartOfDocument
// - atStartOfLine
// - atEndOfDocument
// - atEndOfLine
// - move forward with Wrap
// - move forward with NoWrap
// - move backward
// - gotoNextLine
// - gotoPreviousLine
void MovingCursorTest::testConvenienceApi()
{
  KateDocument doc (false, false, false);
  doc.setText("\n"
              "1\n"
              "22\n"
              "333\n"
              "4444\n"
              "55555");

  // check start and end of document
  KTextEditor::MovingCursor *startOfDoc = doc.newMovingCursor(KTextEditor::Cursor(0, 0));
  KTextEditor::MovingCursor *endOfDoc = doc.newMovingCursor(KTextEditor::Cursor(5, 5));
  QVERIFY(startOfDoc->atStartOfDocument());
  QVERIFY(startOfDoc->atStartOfLine());
  QVERIFY(endOfDoc->atEndOfDocument());
  QVERIFY(endOfDoc->atEndOfLine());

  // set cursor to (2, 2) and then move to the left two times
  KTextEditor::MovingCursor *moving = doc.newMovingCursor(KTextEditor::Cursor(2, 2));
  QVERIFY(moving->atEndOfLine()); // at 2, 2
  QVERIFY(moving->move(-1));   // at 2, 1
  QCOMPARE(moving->toCursor(), KTextEditor::Cursor(2, 1));
  QVERIFY(!moving->atEndOfLine());
  QVERIFY(moving->move(-1));   // at 2, 0
  QCOMPARE(moving->toCursor(), KTextEditor::Cursor(2, 0));
  QVERIFY(moving->atStartOfLine());

  // now move again to the left, should wrap to (1, 1)
  QVERIFY(moving->move(-1));   // at 1, 1
  QCOMPARE(moving->toCursor(), KTextEditor::Cursor(1, 1));
  QVERIFY(moving->atEndOfLine());

  // advance 7 characters to position (3, 3)
  QVERIFY(moving->move(7));   // at 3, 3
  QCOMPARE(moving->toCursor(), KTextEditor::Cursor(3, 3));

  // advance 20 characters in NoWrap mode, then go back 10 characters
  QVERIFY(moving->move(20, KTextEditor::MovingCursor::NoWrap));   // at 3, 23
  QCOMPARE(moving->toCursor(), KTextEditor::Cursor(3, 23));
  QVERIFY(moving->move(-10));   // at 3, 13
  QCOMPARE(moving->toCursor(), KTextEditor::Cursor(3, 13));

  // still at invalid text position. move one char to wrap around
  QVERIFY(!moving->isValidTextPosition());   // at 3, 13
  QVERIFY(moving->move(1));   // at 4, 0
  QCOMPARE(moving->toCursor(), KTextEditor::Cursor(4, 0));

  // moving 11 characters in wrap mode moves to (5, 6), which is not a valid
  // text position anymore. Hence, moving should be rejected.
  QVERIFY(!moving->move(11));
  QVERIFY(moving->move(10));
  QVERIFY(moving->atEndOfDocument());

  // try to move to next line, which fails. then go to previous line
  QVERIFY(!moving->gotoNextLine());
  QVERIFY(moving->gotoPreviousLine());
  QCOMPARE(moving->toCursor(), KTextEditor::Cursor(4, 0));
}


















