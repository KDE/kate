/* This file is part of the KDE libraries
   Copyright (C) 2012 Dominik Haumann <dhaumann kde org>

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

#include "kte_documentcursor.h"
#include "moc_kte_documentcursor.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <katebuffer.h>
#include <kateglobal.h>
#include <kateview.h>
#include <kateconfig.h>
#include <katesearchbar.h>
#include <ktexteditor/movingrange.h>

#include <documentcursor.h>

QTEST_KDEMAIN(DocumentCursorTest, GUI)

using namespace KTextEditor;

DocumentCursorTest::DocumentCursorTest()
  : QObject()
{
}

DocumentCursorTest::~DocumentCursorTest()
{
}

void DocumentCursorTest::initTestCase()
{
  KateGlobal::self()->incRef();
}

void DocumentCursorTest::cleanupTestCase()
{
  KateGlobal::self()->decRef();
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
void DocumentCursorTest::testConvenienceApi()
{
  KateDocument doc (false, false, false);
  doc.setText("\n"
  "1\n"
  "22\n"
  "333\n"
  "4444\n"
  "55555");
  
  // check start and end of document
  DocumentCursor startOfDoc(&doc); startOfDoc.setPosition(Cursor(0, 0));
  DocumentCursor endOfDoc(&doc); endOfDoc.setPosition(Cursor(5, 5));
  QVERIFY(startOfDoc.atStartOfDocument());
  QVERIFY(startOfDoc.atStartOfLine());
  QVERIFY(endOfDoc.atEndOfDocument());
  QVERIFY(endOfDoc.atEndOfLine());
  
  // set cursor to (2, 2) and then move to the left two times
  DocumentCursor moving(&doc); moving.setPosition(Cursor(2, 2));
  QVERIFY(moving.atEndOfLine()); // at 2, 2
  QVERIFY(moving.move(-1));   // at 2, 1
  QCOMPARE(moving.toCursor(), Cursor(2, 1));
  QVERIFY(!moving.atEndOfLine());
  QVERIFY(moving.move(-1));   // at 2, 0
  QCOMPARE(moving.toCursor(), Cursor(2, 0));
  QVERIFY(moving.atStartOfLine());
  
  // now move again to the left, should wrap to (1, 1)
  QVERIFY(moving.move(-1));   // at 1, 1
  QCOMPARE(moving.toCursor(), Cursor(1, 1));
  QVERIFY(moving.atEndOfLine());
  
  // advance 7 characters to position (3, 3)
  QVERIFY(moving.move(7));   // at 3, 3
  QCOMPARE(moving.toCursor(), Cursor(3, 3));
  
  // advance 20 characters in NoWrap mode, then go back 10 characters
  QVERIFY(moving.move(20, DocumentCursor::NoWrap));   // at 3, 23
  QCOMPARE(moving.toCursor(), Cursor(3, 23));
  QVERIFY(moving.move(-10));   // at 3, 13
  QCOMPARE(moving.toCursor(), Cursor(3, 13));
  
  // still at invalid text position. move one char to wrap around
  QVERIFY(!moving.isValidTextPosition());   // at 3, 13
  QVERIFY(moving.move(1));   // at 4, 0
  QCOMPARE(moving.toCursor(), Cursor(4, 0));
  
  // moving 11 characters in wrap mode moves to (5, 6), which is not a valid
  // text position anymore. Hence, moving should be rejected.
  QVERIFY(!moving.move(11));
  QVERIFY(moving.move(10));
  QVERIFY(moving.atEndOfDocument());
  
  // try to move to next line, which fails. then go to previous line
  QVERIFY(!moving.gotoNextLine());
  QVERIFY(moving.gotoPreviousLine());
  QCOMPARE(moving.toCursor(), Cursor(4, 0));
}

void DocumentCursorTest::testOperators()
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
  
  DocumentCursor m02(&doc);
  DocumentCursor m04(&doc);
  DocumentCursor m14(&doc);

  QVERIFY(m02 == invalid);
  QVERIFY(m04 == invalid);
  QVERIFY(m14 == invalid);
  
  m02.setPosition(Cursor(0, 2));
  m04.setPosition(Cursor(0, 4));
  m14.setPosition(Cursor(1, 4));

  // invalid comparison
  //cppcheck-suppress duplicateExpression
  QVERIFY(invalid == invalid);
  QVERIFY(invalid <= c02);
  QVERIFY(invalid < c02);
  QVERIFY(!(invalid > c02));
  QVERIFY(!(invalid >= c02));
  
  QVERIFY(!(invalid == m02));
  QVERIFY(invalid <= m02);
  QVERIFY(invalid < m02);
  QVERIFY(!(invalid > m02));
  QVERIFY(!(invalid >= m02));
  
  QVERIFY(!(m02 == invalid));
  QVERIFY(!(m02 <= invalid));
  QVERIFY(!(m02 < invalid));
  QVERIFY(m02 > invalid);
  QVERIFY(m02 >= invalid);
  
  // DocumentCursor <-> DocumentCursor
  //cppcheck-suppress duplicateExpression
  QVERIFY(m02 == m02);
  //cppcheck-suppress duplicateExpression
  QVERIFY(m02 <= m02);
  //cppcheck-suppress duplicateExpression
  QVERIFY(m02 >= m02);
  //cppcheck-suppress duplicateExpression
  QVERIFY(!(m02 < m02));
  //cppcheck-suppress duplicateExpression
  QVERIFY(!(m02 > m02));
  //cppcheck-suppress duplicateExpression
  QVERIFY(!(m02 != m02));
  
  QVERIFY(!(m02 == m04));
  QVERIFY(m02 <= m04);
  QVERIFY(!(m02 >= m04));
  QVERIFY(m02 < m04);
  QVERIFY(!(m02 > m04));
  QVERIFY(m02 != m04);
  
  QVERIFY(!(m04 == m02));
  QVERIFY(!(m04 <= m02));
  QVERIFY(m04 >= m02);
  QVERIFY(!(m04 < m02));
  QVERIFY(m04 > m02);
  QVERIFY(m04 != m02);
  
  QVERIFY(!(m02 == m14));
  QVERIFY(m02 <= m14);
  QVERIFY(!(m02 >= m14));
  QVERIFY(m02 < m14);
  QVERIFY(!(m02 > m14));
  QVERIFY(m02 != m14);
  
  QVERIFY(!(m14 == m02));
  QVERIFY(!(m14 <= m02));
  QVERIFY(m14 >= m02);
  QVERIFY(!(m14 < m02));
  QVERIFY(m14 > m02);
  QVERIFY(m14 != m02);
  
  // DocumentCursor <-> Cursor
  QVERIFY(m02 == c02);
  QVERIFY(m02 <= c02);
  QVERIFY(m02 >= c02);
  QVERIFY(!(m02 < c02));
  QVERIFY(!(m02 > c02));
  QVERIFY(!(m02 != c02));
  
  QVERIFY(!(m02 == c04));
  QVERIFY(m02 <= c04);
  QVERIFY(!(m02 >= c04));
  QVERIFY(m02 < c04);
  QVERIFY(!(m02 > c04));
  QVERIFY(m02 != c04);
  
  QVERIFY(!(m04 == c02));
  QVERIFY(!(m04 <= c02));
  QVERIFY(m04 >= c02);
  QVERIFY(!(m04 < c02));
  QVERIFY(m04 > c02);
  QVERIFY(m04 != c02);
  
  QVERIFY(!(m02 == c14));
  QVERIFY(m02 <= c14);
  QVERIFY(!(m02 >= c14));
  QVERIFY(m02 < c14);
  QVERIFY(!(m02 > c14));
  QVERIFY(m02 != c14);
  
  QVERIFY(!(m14 == c02));
  QVERIFY(!(m14 <= c02));
  QVERIFY(m14 >= c02);
  QVERIFY(!(m14 < c02));
  QVERIFY(m14 > c02);
  QVERIFY(m14 != c02);
  
  // Cursor <-> DocumentCursor
  QVERIFY(c02 == m02);
  QVERIFY(c02 <= m02);
  QVERIFY(c02 >= m02);
  QVERIFY(!(c02 < m02));
  QVERIFY(!(c02 > m02));
  QVERIFY(!(c02 != m02));
  
  QVERIFY(!(c02 == m04));
  QVERIFY(c02 <= m04);
  QVERIFY(!(c02 >= m04));
  QVERIFY(c02 < m04);
  QVERIFY(!(c02 > m04));
  QVERIFY(c02 != m04);
  
  QVERIFY(!(c04 == m02));
  QVERIFY(!(c04 <= m02));
  QVERIFY(c04 >= m02);
  QVERIFY(!(c04 < m02));
  QVERIFY(c04 > m02);
  QVERIFY(c04 != m02);
  
  QVERIFY(!(c02 == m14));
  QVERIFY(c02 <= m14);
  QVERIFY(!(c02 >= m14));
  QVERIFY(c02 < m14);
  QVERIFY(!(c02 > m14));
  QVERIFY(c02 != m14);
  
  QVERIFY(!(c14 == m02));
  QVERIFY(!(c14 <= m02));
  QVERIFY(c14 >= m02);
  QVERIFY(!(c14 < m02));
  QVERIFY(c14 > m02);
  QVERIFY(c14 != m02);
}
