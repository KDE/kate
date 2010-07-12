/* This file is part of the KDE libraries
   Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#include "range_test.h"
#include "moc_range_test.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <kateview.h>
#include <kateconfig.h>
#include <ktexteditor/movingcursor.h>
#include <ktexteditor/movingrange.h>

QTEST_KDEMAIN(RangeTest, GUI)

namespace QTest {
  template<>
  char *toString(const KTextEditor::Range &range)
  {
    QByteArray ba = "Range[";
    ba += QByteArray::number(range.start().line()) + ", " + QByteArray::number(range.start().column()) + ", ";
    ba += QByteArray::number(range.end().line())   + ", " + QByteArray::number(range.end().column());
    ba += "]";
    return qstrdup(ba.data());
  }
}

#define testNewRow() (QTest::newRow(QString("line %1").arg(__LINE__).toAscii().data()))

RangeTest::RangeTest()
    : QObject()
{
}

RangeTest::~RangeTest()
{
}

void RangeTest::rangeCheck ( KTextEditor::Range & valid )
{
  QVERIFY(valid.isValid() && valid.start() <= valid.end());

  KTextEditor::Cursor before(0,1), start(0,2), end(1,4), after(1,10);

  KTextEditor::Range result(start, end);
  QVERIFY(valid.isValid() && valid.start() <= valid.end());

  valid.setRange(start, end);
  QVERIFY(valid.isValid() && valid.start() <= valid.end());
  QCOMPARE(valid, result);

  valid.setRange(end, start);
  QVERIFY(valid.isValid() && valid.start() <= valid.end());
  QCOMPARE(valid, result);

  valid.start() = after;
  QVERIFY(valid.isValid() && valid.start() <= valid.end());
  QCOMPARE(valid, KTextEditor::Range(after, after));

  valid = result;
  QCOMPARE(valid, result);

  valid.end() = before;
  QVERIFY(valid.isValid() && valid.start() <= valid.end());
  QCOMPARE(valid, KTextEditor::Range(before, before));
}

void RangeTest::testTextEditorRange()
{
  // test simple range
  KTextEditor::Range range;
  rangeCheck (range);
}

void RangeTest::testTextRange()
{
  // test text range
  KateDocument doc (false, false, false);
  KTextEditor::MovingRange *complexRange = doc.newMovingRange (KTextEditor::Range());
  KTextEditor::Range range = *complexRange;
  rangeCheck (range);
  delete complexRange;
}

void RangeTest::testInsertText()
{
  KateDocument doc (false, false, false);

  // Multi-line insert
  KTextEditor::MovingCursor* cursor1 = doc.newMovingCursor(KTextEditor::Cursor(), KTextEditor::MovingCursor::StayOnInsert);
  KTextEditor::MovingCursor* cursor2 = doc.newMovingCursor(KTextEditor::Cursor(), KTextEditor::MovingCursor::MoveOnInsert);

  doc.insertText(KTextEditor::Cursor(), "Test Text\nMore Test Text");
  QCOMPARE(doc.documentEnd(), KTextEditor::Cursor(1,14));

  QString text = doc.text(KTextEditor::Range(1,0,1,14));
  QCOMPARE(text, QString("More Test Text"));

  // Check cursors and ranges have moved properly
  QCOMPARE(cursor1->toCursor(), KTextEditor::Cursor(0,0));
  QCOMPARE(cursor2->toCursor(), KTextEditor::Cursor(1,14));

  KTextEditor::Cursor cursor3 = doc.endOfLine(1);

  // Set up a few more lines
  doc.insertText(*cursor2, "\nEven More Test Text");
  QCOMPARE(doc.documentEnd(), KTextEditor::Cursor(2,19));
  QCOMPARE(cursor3, doc.endOfLine(1));
}

void RangeTest::testCornerCaseInsertion()
{
  KateDocument doc (false, false, false);

  // lock first revision
  doc.lockRevision (0);

  KTextEditor::MovingRange* rangeEdit = doc.newMovingRange(KTextEditor::Range(0,0,0,0));
  QCOMPARE(rangeEdit->toRange(), KTextEditor::Range(0,0,0,0));

  doc.insertText(KTextEditor::Cursor(0,0), "\n");
  QCOMPARE(rangeEdit->toRange(), KTextEditor::Range(1,0,1,0));

  // test translate
  KTextEditor::Range translateTest (0,0,0,0);
  doc.transformRange (translateTest, KTextEditor::MovingRange::DoNotExpand, KTextEditor::MovingRange::AllowEmpty, 0);
  QCOMPARE(translateTest, KTextEditor::Range(1,0,1,0));
  
  // test translate reverse
  KTextEditor::Range reverseTranslateTest (1,0,1,0);
  doc.transformRange (reverseTranslateTest, KTextEditor::MovingRange::DoNotExpand, KTextEditor::MovingRange::AllowEmpty, -1, 0);
  QCOMPARE(reverseTranslateTest, KTextEditor::Range(0,0,0,0));
}
