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
#include <ktexteditor/movingcursor.h>

using namespace KTextEditor;

QTEST_KDEMAIN(MovingCursorTest, GUI)

namespace QTest {
    template<>
    char *toString(const KTextEditor::Cursor &cursor)
    {
        QByteArray ba = "Cursor[" + QByteArray::number(cursor.line())
        + ", " + QByteArray::number(cursor.column()) + "]";
        return qstrdup(ba.data());
    }
}

MovingCursorTest::MovingCursorTest()
  : QObject()
{
}

MovingCursorTest::~MovingCursorTest()
{
}

// tests:
// - MovingCursor with StayOnInsert
// - MovingCursor with MoveOnInsert
void MovingCursorTest::testMovingCursor()
{
  KateDocument doc (false, false, false);
  MovingCursor* invalid = doc.newMovingCursor(Cursor::invalid());
  MovingCursor* moveOnInsert = doc.newMovingCursor(Cursor(0, 0), MovingCursor::MoveOnInsert);
  MovingCursor* stayOnInsert = doc.newMovingCursor(Cursor(0, 0), MovingCursor::StayOnInsert);

  // verify initial conditions
  QVERIFY(!invalid->isValid());
  QCOMPARE(moveOnInsert->toCursor(), Cursor(0, 0));
  QCOMPARE(stayOnInsert->toCursor(), Cursor(0, 0));

  // insert some text
  doc.insertText(Cursor(0, 0), "\n"
                                "1\n"
                                "22");

  // check new cursor positions
  QCOMPARE(moveOnInsert->toCursor(), Cursor(2, 2));
  QCOMPARE(stayOnInsert->toCursor(), Cursor(0, 0));

  // set position to (1, 1) and insert text before cursor
  stayOnInsert->setPosition(Cursor(1, 1));
  QCOMPARE(stayOnInsert->toCursor(), Cursor(1, 1));
  doc.insertText(Cursor(1, 0), "test");
  QCOMPARE(stayOnInsert->toCursor(), Cursor(1, 5));
  doc.undo();
  QCOMPARE(stayOnInsert->toCursor(), Cursor(1, 1));

  // position still at (1, 1). insert text at cursor
  doc.insertText(Cursor(1, 1), "test");
  QCOMPARE(stayOnInsert->toCursor(), Cursor(1, 1));
  doc.undo();
  QCOMPARE(stayOnInsert->toCursor(), Cursor(1, 1));

  //
  // same tests with the moveOnInsert cursor
  //
  // set position to (1, 1) and insert text before cursor
  moveOnInsert->setPosition(Cursor(1, 1));
  QCOMPARE(moveOnInsert->toCursor(), Cursor(1, 1));
  doc.insertText(Cursor(1, 0), "test");
  QCOMPARE(moveOnInsert->toCursor(), Cursor(1, 5));
  doc.undo();
  QCOMPARE(moveOnInsert->toCursor(), Cursor(1, 1));

  // position still at (1, 1). insert text at cursor
  doc.insertText(Cursor(1, 1), "test");
  QCOMPARE(moveOnInsert->toCursor(), Cursor(1, 5));
  doc.undo();
  QCOMPARE(moveOnInsert->toCursor(), Cursor(1, 1));

  // set both cursors to (2, 1) then delete text range that contains cursors
  moveOnInsert->setPosition(Cursor(2, 1));
  stayOnInsert->setPosition(Cursor(2, 1));
  doc.removeText(Range(Cursor(2, 0), Cursor(2, 2)));
  QCOMPARE(moveOnInsert->toCursor(), Cursor(2, 0));
  QCOMPARE(moveOnInsert->toCursor(), Cursor(2, 0));
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
  MovingCursor *startOfDoc = doc.newMovingCursor(Cursor(0, 0));
  MovingCursor *endOfDoc = doc.newMovingCursor(Cursor(5, 5));
  QVERIFY(startOfDoc->atStartOfDocument());
  QVERIFY(startOfDoc->atStartOfLine());
  QVERIFY(endOfDoc->atEndOfDocument());
  QVERIFY(endOfDoc->atEndOfLine());

  // set cursor to (2, 2) and then move to the left two times
  MovingCursor *moving = doc.newMovingCursor(Cursor(2, 2));
  QVERIFY(moving->atEndOfLine()); // at 2, 2
  QVERIFY(moving->move(-1));   // at 2, 1
  QCOMPARE(moving->toCursor(), Cursor(2, 1));
  QVERIFY(!moving->atEndOfLine());
  QVERIFY(moving->move(-1));   // at 2, 0
  QCOMPARE(moving->toCursor(), Cursor(2, 0));
  QVERIFY(moving->atStartOfLine());

  // now move again to the left, should wrap to (1, 1)
  QVERIFY(moving->move(-1));   // at 1, 1
  QCOMPARE(moving->toCursor(), Cursor(1, 1));
  QVERIFY(moving->atEndOfLine());

  // advance 7 characters to position (3, 3)
  QVERIFY(moving->move(7));   // at 3, 3
  QCOMPARE(moving->toCursor(), Cursor(3, 3));

  // advance 20 characters in NoWrap mode, then go back 10 characters
  QVERIFY(moving->move(20, MovingCursor::NoWrap));   // at 3, 23
  QCOMPARE(moving->toCursor(), Cursor(3, 23));
  QVERIFY(moving->move(-10));   // at 3, 13
  QCOMPARE(moving->toCursor(), Cursor(3, 13));

  // still at invalid text position. move one char to wrap around
  QVERIFY(!moving->isValidTextPosition());   // at 3, 13
  QVERIFY(moving->move(1));   // at 4, 0
  QCOMPARE(moving->toCursor(), Cursor(4, 0));

  // moving 11 characters in wrap mode moves to (5, 6), which is not a valid
  // text position anymore. Hence, moving should be rejected.
  QVERIFY(!moving->move(11));
  QVERIFY(moving->move(10));
  QVERIFY(moving->atEndOfDocument());

  // try to move to next line, which fails. then go to previous line
  QVERIFY(!moving->gotoNextLine());
  QVERIFY(moving->gotoPreviousLine());
  QCOMPARE(moving->toCursor(), Cursor(4, 0));
}

void MovingCursorTest::testOperators()
{
    KateDocument doc (false, false, false);
    doc.setText("--oo--\n"
                "--oo--\n"
                "--oo--");

    // create lots of cursors for comparison
    Cursor invalid = Cursor::invalid();
    Cursor c02(0, 2);
    Cursor c04(0, 4);
    Cursor c14(1, 4);

    MovingCursor* m02 = doc.newMovingCursor(Cursor(0, 2));
    MovingCursor* m04 = doc.newMovingCursor(Cursor(0, 4));
    MovingCursor* m14 = doc.newMovingCursor(Cursor(1, 4));

    // invalid comparison
    QVERIFY(invalid == invalid);
    QVERIFY(invalid <= c02);
    QVERIFY(invalid < c02);
    QVERIFY(!(invalid > c02));
    QVERIFY(!(invalid >= c02));

    QVERIFY(!(invalid == *m02));
    QVERIFY(invalid <= *m02);
    QVERIFY(invalid < *m02);
    QVERIFY(!(invalid > *m02));
    QVERIFY(!(invalid >= *m02));

    QVERIFY(!(*m02 == invalid));
    QVERIFY(!(*m02 <= invalid));
    QVERIFY(!(*m02 < invalid));
    QVERIFY(*m02 > invalid);
    QVERIFY(*m02 >= invalid);

    // MovingCursor <-> MovingCursor
    QVERIFY(*m02 == *m02);
    QVERIFY(*m02 <= *m02);
    QVERIFY(*m02 >= *m02);
    QVERIFY(!(*m02 < *m02));
    QVERIFY(!(*m02 > *m02));
    QVERIFY(!(*m02 != *m02));

    QVERIFY(!(*m02 == *m04));
    QVERIFY(*m02 <= *m04);
    QVERIFY(!(*m02 >= *m04));
    QVERIFY(*m02 < *m04);
    QVERIFY(!(*m02 > *m04));
    QVERIFY(*m02 != *m04);

    QVERIFY(!(*m04 == *m02));
    QVERIFY(!(*m04 <= *m02));
    QVERIFY(*m04 >= *m02);
    QVERIFY(!(*m04 < *m02));
    QVERIFY(*m04 > *m02);
    QVERIFY(*m04 != *m02);

    QVERIFY(!(*m02 == *m14));
    QVERIFY(*m02 <= *m14);
    QVERIFY(!(*m02 >= *m14));
    QVERIFY(*m02 < *m14);
    QVERIFY(!(*m02 > *m14));
    QVERIFY(*m02 != *m14);

    QVERIFY(!(*m14 == *m02));
    QVERIFY(!(*m14 <= *m02));
    QVERIFY(*m14 >= *m02);
    QVERIFY(!(*m14 < *m02));
    QVERIFY(*m14 > *m02);
    QVERIFY(*m14 != *m02);

    // MovingCursor <-> Cursor
    QVERIFY(*m02 == c02);
    QVERIFY(*m02 <= c02);
    QVERIFY(*m02 >= c02);
    QVERIFY(!(*m02 < c02));
    QVERIFY(!(*m02 > c02));
    QVERIFY(!(*m02 != c02));

    QVERIFY(!(*m02 == c04));
    QVERIFY(*m02 <= c04);
    QVERIFY(!(*m02 >= c04));
    QVERIFY(*m02 < c04);
    QVERIFY(!(*m02 > c04));
    QVERIFY(*m02 != c04);

    QVERIFY(!(*m04 == c02));
    QVERIFY(!(*m04 <= c02));
    QVERIFY(*m04 >= c02);
    QVERIFY(!(*m04 < c02));
    QVERIFY(*m04 > c02);
    QVERIFY(*m04 != c02);

    QVERIFY(!(*m02 == c14));
    QVERIFY(*m02 <= c14);
    QVERIFY(!(*m02 >= c14));
    QVERIFY(*m02 < c14);
    QVERIFY(!(*m02 > c14));
    QVERIFY(*m02 != c14);

    QVERIFY(!(*m14 == c02));
    QVERIFY(!(*m14 <= c02));
    QVERIFY(*m14 >= c02);
    QVERIFY(!(*m14 < c02));
    QVERIFY(*m14 > c02);
    QVERIFY(*m14 != c02);

    // Cursor <-> MovingCursor
    QVERIFY(c02 == *m02);
    QVERIFY(c02 <= *m02);
    QVERIFY(c02 >= *m02);
    QVERIFY(!(c02 < *m02));
    QVERIFY(!(c02 > *m02));
    QVERIFY(!(c02 != *m02));

    QVERIFY(!(c02 == *m04));
    QVERIFY(c02 <= *m04);
    QVERIFY(!(c02 >= *m04));
    QVERIFY(c02 < *m04);
    QVERIFY(!(c02 > *m04));
    QVERIFY(c02 != *m04);

    QVERIFY(!(c04 == *m02));
    QVERIFY(!(c04 <= *m02));
    QVERIFY(c04 >= *m02);
    QVERIFY(!(c04 < *m02));
    QVERIFY(c04 > *m02);
    QVERIFY(c04 != *m02);

    QVERIFY(!(c02 == *m14));
    QVERIFY(c02 <= *m14);
    QVERIFY(!(c02 >= *m14));
    QVERIFY(c02 < *m14);
    QVERIFY(!(c02 > *m14));
    QVERIFY(c02 != *m14);

    QVERIFY(!(c14 == *m02));
    QVERIFY(!(c14 <= *m02));
    QVERIFY(c14 >= *m02);
    QVERIFY(!(c14 < *m02));
    QVERIFY(c14 > *m02);
    QVERIFY(c14 != *m02);
}
