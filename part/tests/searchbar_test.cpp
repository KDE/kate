/* This file is part of the KDE libraries
   Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

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

#include "searchbar_test.h"
#include "moc_searchbar_test.cpp"

#include "ui_searchbarincremental.h"
#include "ui_searchbarpower.h"

#include <qtest_kde.h>

#include <katedocument.h>
#include <kateview.h>
#include <kateconfig.h>
#include <katesearchbar.h>
#include <ktexteditor/movingrange.h>

QTEST_KDEMAIN(SearchBarTest, GUI)

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


using namespace KTextEditor;


class TestSearchBar : public KateSearchBar
{
public:
  TestSearchBar(bool initAsPower, KateView *view, KateViewConfig *config)
    : KateSearchBar(initAsPower, view, config)
  {}

  const QList<KTextEditor::MovingRange*> &childRanges() const
  {
    return m_hlRanges;
  }

  Ui::IncrementalSearchBar *incUi() { return m_incUi; }
  Ui::PowerSearchBar *powerUi() { return m_powerUi; }
};


SearchBarTest::SearchBarTest()
    : QObject()
{
}

SearchBarTest::~SearchBarTest()
{
}


void SearchBarTest::testFindNextIncremental()
{
  KateDocument doc(false, false, false);
  doc.setText("a a a b b");

  KateView view(&doc, 0);
  KateViewConfig config(&view);

  KateSearchBar bar(false, &view, &config);

  bar.setSearchPattern("b");

  QCOMPARE(view.selectionRange(), Range(0, 6, 0, 7));

  bar.findNext();

  QCOMPARE(view.selectionRange(), Range(0, 8, 0, 9));

  bar.setSearchPattern("a");

  QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

  bar.findNext();

  QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));

  bar.findNext();

  QCOMPARE(view.selectionRange(), Range(0, 4, 0, 5));

  bar.findNext();

  QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));
}


void SearchBarTest::testSetMatchCaseIncremental()
{
  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText("a A a");
  KateSearchBar bar(false, &view, &config);

  QVERIFY(!bar.isPower());
  QVERIFY(!view.selection());

  bar.setMatchCase(false);
  bar.setSearchPattern("A");

  QVERIFY(!bar.matchCase());
  QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

  bar.setMatchCase(true);

  QVERIFY(bar.matchCase());
  QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));

  bar.setMatchCase(false);

  QVERIFY(!bar.matchCase());
  QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

  bar.setMatchCase(true);

  QVERIFY(bar.matchCase());
  QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));
}


void SearchBarTest::testSetMatchCasePower()
{
  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText("a A a");
  view.setCursorPosition(Cursor(0, 0));

  KateSearchBar bar(true, &view, &config);

  QVERIFY(bar.isPower());
  QVERIFY(!view.selection());

  bar.setMatchCase(false);
  bar.setSearchPattern("A");
  bar.findNext();

  QCOMPARE(bar.searchPattern(), QString("A"));
  QVERIFY(!bar.matchCase());
  QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

  bar.setMatchCase(true);

  QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

  bar.findNext();

  QVERIFY(bar.matchCase());
  QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));

  bar.setMatchCase(false);

  QVERIFY(!bar.matchCase());
  QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));

  bar.findNext();

  QCOMPARE(view.selectionRange(), Range(0, 4, 0, 5));
}



void SearchBarTest::testSetSelectionOnlyPower()
{
  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText("a a a");
  KateSearchBar bar(true, &view, &config);

  bar.setSearchPattern("a");

  QVERIFY(bar.isPower());
  QVERIFY(!view.selection());

  bar.setSelectionOnly(false);
  bar.findNext();

  QVERIFY(!bar.selectionOnly());
  QCOMPARE(view.selectionRange(), Range(0, 0, 0, 1));

  view.setSelection(Range(0, 2, 0, 5));
  bar.setSelectionOnly(true);

  QVERIFY(bar.selectionOnly());

  bar.findNext();

  QCOMPARE(view.selectionRange(), Range(0, 2, 0, 3));
  QVERIFY(bar.selectionOnly());

  bar.setSelectionOnly(false);
  bar.findNext();

  QCOMPARE(view.selectionRange(), Range(0, 4, 0, 5));
  QVERIFY(!bar.selectionOnly());
}


void SearchBarTest::testSetSearchPattern_data()
{
  QTest::addColumn<bool>("power");
  QTest::addColumn<int>("numMatches2");

  testNewRow() << false << 0;
  testNewRow() << true  << 3;
}


void SearchBarTest::testSetSearchPattern()
{
  QFETCH(bool, power);
  QFETCH(int, numMatches2);

  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText("a a a");

  TestSearchBar bar(power, &view, &config);

  bar.setSearchPattern("a");
  bar.findAll();

  QCOMPARE(bar.childRanges().size(), 3);

  bar.setSearchPattern("a ");

  QCOMPARE(bar.childRanges().size(), numMatches2);

  bar.findAll();

  QCOMPARE(bar.childRanges().size(), 2);
}


void SearchBarTest::testSetSelectionOnly()
{
  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText("a a a");
  view.setSelection(Range(0, 0, 0, 3));

  TestSearchBar bar(false, &view, &config);

  bar.setSelectionOnly(false);
  bar.setSearchPattern("a");
  bar.findAll();

  QCOMPARE(bar.childRanges().size(), 3);

  bar.setSelectionOnly(true);

  QCOMPARE(bar.childRanges().size(), 3);
}


void SearchBarTest::testFindAll_data()
{
  QTest::addColumn<bool>("power");
  QTest::addColumn<int>("numMatches2");
  QTest::addColumn<int>("numMatches4");

  testNewRow() << false << 0 << 0;
  testNewRow() << true  << 3 << 2;
}


void SearchBarTest::testFindAll()
{
  QFETCH(bool, power);
  QFETCH(int, numMatches2);
  QFETCH(int, numMatches4);

  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText("a a a");
  TestSearchBar bar(power, &view, &config);

  QCOMPARE(bar.isPower(), power);

  bar.setSearchPattern("a");
  bar.findAll();

  QCOMPARE(bar.childRanges().size(), 3);
  QCOMPARE(bar.childRanges().at(0)->toRange(), Range(0, 0, 0, 1));
  QCOMPARE(bar.childRanges().at(1)->toRange(), Range(0, 2, 0, 3));
  QCOMPARE(bar.childRanges().at(2)->toRange(), Range(0, 4, 0, 5));

  bar.setSearchPattern("a ");

  QCOMPARE(bar.childRanges().size(), numMatches2);

  bar.findAll();

  QCOMPARE(bar.childRanges().size(), 2);

  bar.setSearchPattern("a  ");

  QCOMPARE(bar.childRanges().size(), numMatches4);

  bar.findAll();

  QCOMPARE(bar.childRanges().size(), 0);
}

void SearchBarTest::testReplaceAll()
{
  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText("a a a");
  TestSearchBar bar(true, &view, &config);

  bar.setSearchPattern("a");
  bar.setReplacePattern("");
  bar.replaceAll();

  QCOMPARE(bar.childRanges().size(), 3);
  QCOMPARE(bar.childRanges().at(0)->toRange(), Range(0, 0, 0, 0));
  QCOMPARE(bar.childRanges().at(1)->toRange(), Range(0, 1, 0, 1));
  QCOMPARE(bar.childRanges().at(2)->toRange(), Range(0, 2, 0, 2));

  bar.setSearchPattern(" ");
  bar.setReplacePattern("b");
  bar.replaceAll();

  QCOMPARE(bar.childRanges().size(), 2);
  QCOMPARE(bar.childRanges().at(0)->toRange(), Range(0, 0, 0, 1));
  QCOMPARE(bar.childRanges().at(1)->toRange(), Range(0, 1, 0, 2));
}

void SearchBarTest::testFindSelectionForward_data()
{
  QTest::addColumn<QString>("text");
  QTest::addColumn<bool>("selectionOnly");
  QTest::addColumn<Range>("selectionRange");
  QTest::addColumn<Range>("match");

  testNewRow() << "a a a" << false << Range(0, 0, 0, 1) << Range(0, 0, 0, 2);
  testNewRow() << "a a a" << true  << Range(0, 0, 0, 1) << Range(0, 0, 0, 1);

  testNewRow() << "a a a" << false << Range(0, 0, 0, 2) << Range(0, 2, 0, 4);
  testNewRow() << "a a a" << true  << Range(0, 0, 0, 2) << Range(0, 0, 0, 2);

  testNewRow() << "a a a" << false << Range(0, 0, 0, 3) << Range(0, 0, 0, 2);
  testNewRow() << "a a a" << true  << Range(0, 0, 0, 3) << Range(0, 0, 0, 2);

  testNewRow() << "a a a" << false << Range(0, 2, 0, 4) << Range(0, 0, 0, 2);
  testNewRow() << "a a a" << true  << Range(0, 2, 0, 4) << Range(0, 2, 0, 4);

  testNewRow() << "a a a" << false << Range(0, 3, 0, 4) << Range(0, 0, 0, 2);
  testNewRow() << "a a a" << true  << Range(0, 3, 0, 4) << Range(0, 3, 0, 4);
}

void SearchBarTest::testFindSelectionForward()
{
  QFETCH(QString, text);
  QFETCH(bool, selectionOnly);
  QFETCH(Range, selectionRange);
  QFETCH(Range, match);

  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText(text);

  view.setSelection(Range(0, 0, 0, 2));

  KateSearchBar bar(true, &view, &config);
  QVERIFY(bar.searchPattern() == QString("a "));

  view.setSelection(selectionRange);
  QCOMPARE(view.selectionRange(), selectionRange);
  bar.setSelectionOnly(selectionOnly);

  bar.findNext();

  QCOMPARE(view.selectionRange(), match);
}

void SearchBarTest::testRemoveWithSelectionForward_data()
{
  QTest::addColumn<Range>("selectionRange");
  QTest::addColumn<Range>("match");

  testNewRow() << Range(0, 0, 0, 1) << Range(0, 0, 0, 2);
  testNewRow() << Range(0, 0, 0, 2) << Range(0, 0, 0, 2);
  testNewRow() << Range(0, 0, 0, 3) << Range(0, 0, 0, 2);
  testNewRow() << Range(0, 2, 0, 4) << Range(0, 0, 0, 2);
  testNewRow() << Range(0, 3, 0, 4) << Range(0, 0, 0, 2);
}

void SearchBarTest::testRemoveWithSelectionForward()
{
  QFETCH(Range, selectionRange);
  QFETCH(Range, match);

  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText("a a a");
  view.setSelection(selectionRange);

  KateSearchBar bar(true, &view, &config);
  bar.setSearchPattern("a ");
  bar.setSelectionOnly(false);

  bar.replaceNext();

  QCOMPARE(view.selectionRange(), match);
}

void SearchBarTest::testRemoveInSelectionForward_data()
{
  QTest::addColumn<Range>("selectionRange");
  QTest::addColumn<Range>("match");

  testNewRow() << Range(0, 0, 0, 1) << Range(0, 0, 0, 1);
  testNewRow() << Range(0, 0, 0, 2) << Range(0, 0, 0, 0);
  testNewRow() << Range(0, 0, 0, 3) << Range(0, 0, 0, 2);
  testNewRow() << Range(0, 0, 0, 4) << Range(0, 0, 0, 2);
  testNewRow() << Range(0, 2, 0, 4) << Range(0, 2, 0, 2);
  testNewRow() << Range(0, 3, 0, 4) << Range(0, 3, 0, 4);
}

void SearchBarTest::testRemoveInSelectionForward()
{
  QFETCH(Range, selectionRange);
  QFETCH(Range, match);

  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText("a a a");
  view.setSelection(selectionRange);

  KateSearchBar bar(true, &view, &config);
  bar.setSearchPattern("a ");
  bar.setSelectionOnly(true);

  QVERIFY(bar.replacementPattern().isEmpty());

  bar.replaceNext();

  QCOMPARE(view.selectionRange(), match);
}

void SearchBarTest::testReplaceWithDoubleSelecion_data()
{
  QTest::addColumn<QString>("text");
  QTest::addColumn<Range>("selectionRange");
  QTest::addColumn<QString>("result");
  QTest::addColumn<Range>("match");

//  testNewRow() << "a"  << Range(0, 0, 0, 1) << "aa" << Range(?, ?, ?, ?);
  testNewRow() << "aa" << Range(0, 1, 0, 2) << "aaa" << Range(0, 0, 0, 1);
  testNewRow() << "aa" << Range(0, 0, 0, 1) << "aaa" << Range(0, 2, 0, 3);

//  testNewRow() << "ab"  << Range(0, 0, 0, 1) << "aab"  << Range(?, ?, ?, ?);
  testNewRow() << "aab" << Range(0, 0, 0, 1) << "aaab" << Range(0, 2, 0, 3);
  testNewRow() << "aba" << Range(0, 0, 0, 1) << "aaba" << Range(0, 3, 0, 4);

//  testNewRow() << "ab"   << Range(0, 0, 0, 2) << "abab"   << Range(?, ?, ?, ?);
  testNewRow() << "abab" << Range(0, 0, 0, 2) << "ababab" << Range(0, 4, 0, 6);
  testNewRow() << "abab" << Range(0, 2, 0, 4) << "ababab" << Range(0, 0, 0, 2);
}

void SearchBarTest::testReplaceWithDoubleSelecion()
{
  QFETCH(QString, text);
  QFETCH(Range, selectionRange);
  QFETCH(QString, result);
  QFETCH(Range, match);

  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText(text);
  view.setSelection(selectionRange);

  KateSearchBar bar(true, &view, &config);

  bar.setSelectionOnly(false);
  bar.setReplacePattern(bar.searchPattern() + bar.searchPattern());
  bar.replaceNext();

  QCOMPARE(doc.text(), result);
  QCOMPARE(view.selectionRange(), match);
}

void SearchBarTest::testReplaceDollar()
{
  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig config(&view);

  doc.setText("aaa\nbbb\nccc\n\n\naaa\nbbb\nccc\nddd\n");

  KateSearchBar bar(true, &view, &config);

  bar.setSearchPattern("$");
  bar.setSearchMode(KateSearchBar::MODE_REGEX);
  bar.setReplacePattern("D");

  bar.replaceAll();

  QCOMPARE(doc.text(), QString("aaaD\nbbbD\ncccD\nD\nD\naaaD\nbbbD\ncccD\ndddD\n"));
}

void SearchBarTest::testSearchHistoryIncremental()
{
  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig *const config = view.config();

  doc.setText("foo bar");

  QCOMPARE(config->patternHistoryModel()->stringList(), QStringList());

  TestSearchBar bar(false, &view, config);

  bar.setSearchPattern("foo");
  bar.findNext();

  QCOMPARE(bar.incUi()->pattern->findText("foo"), 0);

  bar.setSearchPattern("bar");
  bar.findNext();

  QCOMPARE(bar.incUi()->pattern->findText("bar"), 0);
  QCOMPARE(bar.incUi()->pattern->findText("foo"), 1);

  KateDocument doc2(false, false, false);
  KateView view2(&doc2, 0);
  KateViewConfig *const config2 = view2.config();
  TestSearchBar bar2(false, &view2, config2);

  QCOMPARE(bar2.incUi()->pattern->findText("bar"), 0);
  QCOMPARE(bar2.incUi()->pattern->findText("foo"), 1);
}

void SearchBarTest::testSearchHistoryPower()
{
  KateDocument doc(false, false, false);
  KateView view(&doc, 0);
  KateViewConfig *const config = view.config();

  doc.setText("foo bar");

  TestSearchBar bar(true, &view, config);

  QCOMPARE(bar.powerUi()->pattern->count(), 0);

  bar.setSearchPattern("foo");
  bar.findNext();

  QCOMPARE(bar.powerUi()->pattern->findText("foo"), 0);

  bar.findNext();

  QCOMPARE(bar.powerUi()->pattern->findText("foo"), 0);
  QCOMPARE(bar.powerUi()->pattern->count(), 1);

  bar.setSearchPattern("bar");
  bar.findNext();

  QCOMPARE(bar.powerUi()->pattern->findText("bar"), 0);
  QCOMPARE(bar.powerUi()->pattern->findText("foo"), 1);
  QCOMPARE(bar.powerUi()->pattern->count(), 2);

  KateDocument doc2(false, false, false);
  KateView view2(&doc2, 0);
  KateViewConfig *const config2 = view2.config();
  TestSearchBar bar2(true, &view2, config2);

  QCOMPARE(bar2.powerUi()->pattern->findText("bar"), 0);
  QCOMPARE(bar2.powerUi()->pattern->findText("foo"), 1);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
