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

#include "movingrange_test.h"
#include "moc_movingrange_test.cpp"

#include <qtest_kde.h>
#include <qtestmouse.h>

#include <katedocument.h>
#include <kateview.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/movingrangefeedback.h>

using namespace KTextEditor;

QTEST_KDEMAIN(MovingRangeTest, GUI)

namespace QTest {
    template<>
    char *toString(const KTextEditor::Cursor &cursor)
    {
        QByteArray ba = "Cursor[" + QByteArray::number(cursor.line())
        + ", " + QByteArray::number(cursor.column()) + "]";
        return qstrdup(ba.data());
    }
}


class RangeFeedback : public MovingRangeFeedback
{
  public:
    RangeFeedback() : MovingRangeFeedback() { reset(); }

    virtual void rangeEmpty (MovingRange* range) {
      m_rangeEmptyCalled = true;
    }

    virtual void rangeInvalid (MovingRange* range) {
      m_rangeInvalidCalled = true;
    }

    virtual void mouseEnteredRange (MovingRange* range, View* view) {
      m_mouseEnteredRangeCalled = true;
    }

    virtual void mouseExitedRange (MovingRange* range, View* view) {
      m_mouseExitedRangeCalled = true;
    }

    virtual void caretEnteredRange (MovingRange* range, View* view) {
      m_caretEnteredRangeCalled = true;
    }

    virtual void caretExitedRange (MovingRange* range, View* view) {
      m_caretExitedRangeCalled = true;
    }
    
  //
  // Test functions to reset feedback watcher
  //
  public:
    void reset() {
      m_rangeEmptyCalled = false;
      m_rangeInvalidCalled = false;
      m_mouseEnteredRangeCalled = false;
      m_mouseExitedRangeCalled = false;
      m_caretEnteredRangeCalled = false;
      m_caretExitedRangeCalled = false;
    }
    
    void verifyReset() {
      QVERIFY(!m_rangeEmptyCalled);
      QVERIFY(!m_rangeInvalidCalled);
      QVERIFY(!m_mouseEnteredRangeCalled);
      QVERIFY(!m_mouseExitedRangeCalled);
      QVERIFY(!m_caretEnteredRangeCalled);
      QVERIFY(!m_caretExitedRangeCalled);
    }

    bool rangeEmptyCalled() const { return m_rangeEmptyCalled; }
    bool rangeInvalidCalled() const { return m_rangeInvalidCalled; }
    bool mouseEnteredRangeCalled() const { return m_mouseEnteredRangeCalled; }
    bool mouseExitedRangeCalled() const { return m_mouseExitedRangeCalled; }
    bool caretEnteredRangeCalled() const { return m_caretEnteredRangeCalled; }
    bool caretExitedRangeCalled() const { return m_caretExitedRangeCalled; }

  private:
    bool m_rangeEmptyCalled;
    bool m_rangeInvalidCalled;
    bool m_mouseEnteredRangeCalled;
    bool m_mouseExitedRangeCalled;
    bool m_caretEnteredRangeCalled;
    bool m_caretExitedRangeCalled;
};


MovingRangeTest::MovingRangeTest()
  : QObject()
{
}

MovingRangeTest::~MovingRangeTest()
{
}

// tests:
// - RangeFeedback::rangeEmpty
void MovingRangeTest::testFeedbackEmptyRange()
{
  KateDocument doc (false, false, false);
  // the range created below will span the 'x' characters
  QString text("..xxxx\n"
               "xxxx..");
  doc.setText(text);

  // create range feedback
  RangeFeedback rf;

  // allow empty
  MovingRange* range = doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)),
                                          KTextEditor::MovingRange::DoNotExpand,
                                          KTextEditor::MovingRange::AllowEmpty);
  range->setFeedback(&rf);
  rf.verifyReset();
  doc.removeText(range->toRange());
  QVERIFY(rf.rangeEmptyCalled());
  QVERIFY(!rf.rangeInvalidCalled());
  QVERIFY(!rf.mouseEnteredRangeCalled());
  QVERIFY(!rf.mouseExitedRangeCalled());
  QVERIFY(!rf.caretEnteredRangeCalled());
  QVERIFY(!rf.caretExitedRangeCalled());

  // clear document, should call rangeEmpty again
  rf.reset();
  rf.verifyReset();
  doc.clear();
  QVERIFY(rf.rangeEmptyCalled());
  QVERIFY(!rf.rangeInvalidCalled());
  QVERIFY(!rf.mouseEnteredRangeCalled());
  QVERIFY(!rf.mouseExitedRangeCalled());
  QVERIFY(!rf.caretEnteredRangeCalled());
  QVERIFY(!rf.caretExitedRangeCalled());
}

// tests:
// - RangeFeedback::rangeInvalid
void MovingRangeTest::testFeedbackInvalidRange()
{
  KateDocument doc (false, false, false);
  // the range created below will span the 'x' characters
  QString text("..xxxx\n"
               "xxxx..");
  doc.setText(text);

  // create range feedback
  RangeFeedback rf;

  // allow empty
  MovingRange* range = doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)),
                                          KTextEditor::MovingRange::DoNotExpand,
                                          KTextEditor::MovingRange::InvalidateIfEmpty);
  range->setFeedback(&rf);
  rf.verifyReset();
  doc.removeText(range->toRange());
  QVERIFY(!rf.rangeEmptyCalled()); // FIXME: cullmann? should this also be called?
  QVERIFY(rf.rangeInvalidCalled());
  QVERIFY(!rf.mouseEnteredRangeCalled());
  QVERIFY(!rf.mouseExitedRangeCalled());
  QVERIFY(!rf.caretEnteredRangeCalled());
  QVERIFY(!rf.caretExitedRangeCalled());
}

// tests:
// - RangeFeedback::caretEnteredRange
// - RangeFeedback::caretExitedRange
void MovingRangeTest::testFeedbackCaret()
{
  KateDocument doc (false, false, false);
  // the range created below will span the 'x' characters
  QString text("..xxxx\n"
               "xxxx..");
  doc.setText(text);
  
  KateView* view = static_cast<KateView*>(doc.createView(0));

  // create range feedback
  RangeFeedback rf;

  // first test: with ExpandLeft | ExpandRight
  {
    view->setCursorPosition(Cursor(1, 6));

    MovingRange* range = doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)),
                                            KTextEditor::MovingRange::ExpandLeft |
                                            KTextEditor::MovingRange::ExpandRight,
                                            KTextEditor::MovingRange::InvalidateIfEmpty);
    rf.reset();
    range->setFeedback(&rf);
    rf.verifyReset();

    // left
    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(1, 5));
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());
    
    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(1, 4));
    QVERIFY(rf.caretEnteredRangeCalled()); // ExpandRight: include cursor already now
    QVERIFY(!rf.caretExitedRangeCalled());

    rf.reset();
    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(1, 3));
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    rf.reset();
    view->up();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    rf.reset();
    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 2));
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    rf.reset();
    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 1)); // ExpandLeft: now we left it, not before
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(rf.caretExitedRangeCalled());
    
    delete range;
  }
  
  
  // second test: with DoNotExpand
  {
    view->setCursorPosition(Cursor(1, 6));

    MovingRange* range = doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)),
                                            KTextEditor::MovingRange::DoNotExpand,
                                            KTextEditor::MovingRange::InvalidateIfEmpty);
    rf.reset();
    range->setFeedback(&rf);
    rf.verifyReset();

    // left
    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(1, 5));
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());
    
    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(1, 4));
    QVERIFY(!rf.caretEnteredRangeCalled()); // DoNotExpand: does not include cursor
    QVERIFY(!rf.caretExitedRangeCalled());

    rf.reset();
    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(1, 3));
    QVERIFY(rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    rf.reset();
    view->up();
    QCOMPARE(view->cursorPosition(), Cursor(0, 3));
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    rf.reset();
    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 2));
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(rf.caretExitedRangeCalled()); // DoNotExpand: that's why we leave already now

    rf.reset();
    view->cursorLeft();
    QCOMPARE(view->cursorPosition(), Cursor(0, 1));
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());
    
    delete range;
  }
}

// tests:
// - RangeFeedback::mouseEnteredRange
// - RangeFeedback::mouseExitedRange
void MovingRangeTest::testFeedbackMouse()
{
  KateDocument doc (false, false, false);
  // the range created below will span the 'x' characters
  QString text("..xxxx\n"
               "xxxx..");
  doc.setText(text);

  KateView* view = static_cast<KateView*>(doc.createView(0));
  view->setCursorPosition(Cursor(1, 6));
  view->show();
  view->resize(200, 100);

  // create range feedback
  RangeFeedback rf;
  QVERIFY(!rf.mouseEnteredRangeCalled());
  QVERIFY(!rf.mouseExitedRangeCalled());

  // allow empty
  MovingRange* range = doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)),
                                          KTextEditor::MovingRange::ExpandLeft |
                                          KTextEditor::MovingRange::ExpandRight,
                                          KTextEditor::MovingRange::InvalidateIfEmpty);
  range->setFeedback(&rf);
  rf.verifyReset();

  // left (nothing)
  QTest::mouseMove(view, view->cursorToCoordinate(Cursor(0, 0)) + QPoint(0, 5));
  QTest::qWait(200); // process mouse events. do not move mouse manually
  QVERIFY(!rf.mouseEnteredRangeCalled());
  QVERIFY(!rf.mouseExitedRangeCalled());

  // middle (enter)
  rf.reset();
  QTest::mouseMove(view, view->cursorToCoordinate(Cursor(0, 3)) + QPoint(0, 5));
  QTest::qWait(200); // process mouse events. do not move mouse manually
  QVERIFY(rf.mouseEnteredRangeCalled());
  QVERIFY(!rf.mouseExitedRangeCalled());

  // right (exit)
  rf.reset();
  QTest::mouseMove(view, view->cursorToCoordinate(Cursor(1, 6)) + QPoint(10, 5));
  QTest::qWait(200); // process mouse events. do not move mouse manually
  QVERIFY(!rf.mouseEnteredRangeCalled());
  QVERIFY(rf.mouseExitedRangeCalled());
}
