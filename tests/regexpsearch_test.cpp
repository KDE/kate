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

#include "regexpsearch_test.h"
#include "moc_regexpsearch_test.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <kateregexpsearch.h>

Q_DECLARE_METATYPE(KTextEditor::Range)

QTEST_KDEMAIN(RegExpSearchTest, GUI)

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

RegExpSearchTest::RegExpSearchTest()
  : QObject()
{
}

RegExpSearchTest::~RegExpSearchTest()
{
}

void RegExpSearchTest::testReplaceEscapeSequences_data()
{
  QTest::addColumn<QString>("pattern");
  QTest::addColumn<QString>("expected");

  testNewRow() << "\\"       << "\\";
  testNewRow() << "\\0"      << "0";
  testNewRow() << "\\00"     << "00";
  testNewRow() << "\\000"    << "000";
  testNewRow() << "\\0000"   << QString(QChar(0));
  testNewRow() << "\\0377"   << QString(QChar(0377));
  testNewRow() << "\\0378"   << "0378";
  testNewRow() << "\\a"      << "\a";
  testNewRow() << "\\f"      << "\f";
  testNewRow() << "\\n"      << "\n";
  testNewRow() << "\\r"      << "\r";
  testNewRow() << "\\t"      << "\t";
  testNewRow() << "\\v"      << "\v";
  testNewRow() << "\\x"      << "x";
  testNewRow() << "\\x0"     << "x0";
  testNewRow() << "\\x00"    << "x00";
  testNewRow() << "\\x000"   << "x000";
  testNewRow() << "\\x0000"  << QString(QChar(0x0000));
  testNewRow() << "\\x00000" << QString(QChar(0x0000) + '0');
  testNewRow() << "\\xaaaa"  << QString(QChar(0xaaaa));
  testNewRow() << "\\xFFFF"  << QString(QChar(0xFFFF));
  testNewRow() << "\\xFFFg"  << "xFFFg";
}

void RegExpSearchTest::testReplaceEscapeSequences()
{
  QFETCH(QString, pattern);
  QFETCH(QString, expected);

  const QString result1 = KateRegExpSearch::escapePlaintext(pattern);
  const QString result2 = KateRegExpSearch::buildReplacement(pattern, QStringList(), 0);

  QCOMPARE(result1, expected);
  QCOMPARE(result2, expected);
}

void RegExpSearchTest::testReplacementReferences_data()
{
  QTest::addColumn<QString>("pattern");
  QTest::addColumn<QString>("expected");
  QTest::addColumn<QStringList>("capturedTexts");

  testNewRow() << "\\0"    << "b"               << (QStringList() << "b");
  testNewRow() << "\\00"   << "b0"              << (QStringList() << "b");
  testNewRow() << "\\000"  << "b00"             << (QStringList() << "b");
  testNewRow() << "\\0000" << QString(QChar(0)) << (QStringList() << "b");
  testNewRow() << "\\1"    << "1"               << (QStringList() << "b");
  testNewRow() << "\\0"    << "b"               << (QStringList() << "b" << "c");
  testNewRow() << "\\1"    << "c"               << (QStringList() << "b" << "c");
}

void RegExpSearchTest::testReplacementReferences()
{
  QFETCH(QString, pattern);
  QFETCH(QString, expected);
  QFETCH(QStringList, capturedTexts);

  const QString result = KateRegExpSearch::buildReplacement(pattern, capturedTexts, 1);

  QCOMPARE(result, expected);
}

void RegExpSearchTest::testReplacementCaseConversion_data()
{
  QTest::addColumn<QString>("pattern");
  QTest::addColumn<QString>("expected");

  testNewRow() << "a\\Uaa" << "aAA";
  testNewRow() << "a\\UAa" << "aAA";
  testNewRow() << "a\\UaA" << "aAA";

  testNewRow() << "a\\uaa" << "aAa";
  testNewRow() << "a\\uAa" << "aAa";
  testNewRow() << "a\\uaA" << "aAA";

  testNewRow() << "A\\LAA" << "Aaa";
  testNewRow() << "A\\LaA" << "Aaa";
  testNewRow() << "A\\LAa" << "Aaa";

  testNewRow() << "A\\lAA" << "AaA";
  testNewRow() << "A\\lAa" << "Aaa";
  testNewRow() << "A\\laA" << "AaA";

  testNewRow() << "a\\EaA" << "aaA";
  testNewRow() << "A\\EAa" << "AAa";
}

void RegExpSearchTest::testReplacementCaseConversion()
{
  QFETCH(QString, pattern);
  QFETCH(QString, expected);

  const QString result = KateRegExpSearch::buildReplacement(pattern, QStringList(), 1);

  QCOMPARE(result, expected);
}

void RegExpSearchTest::testReplacementCounter_data()
{
  QTest::addColumn<QString>("pattern");
  QTest::addColumn<int>("counter");
  QTest::addColumn<QString>("expected");

  testNewRow() << "a\\#b"     <<  1 << "a1b";
  testNewRow() << "a\\#b"     << 10 << "a10b";
  testNewRow() << "a\\#####b" <<  1 << "a00001b";
}

void RegExpSearchTest::testReplacementCounter()
{
  QFETCH(QString, pattern);
  QFETCH(int, counter);
  QFETCH(QString, expected);

  const QString result = KateRegExpSearch::buildReplacement(pattern, QStringList(), counter);

  QCOMPARE(result, expected);
}

void RegExpSearchTest::testAnchoredRegexp_data()
{
  QTest::addColumn<QString>("pattern");
  QTest::addColumn<Range>("inputRange");
  QTest::addColumn<bool>("backwards");
  QTest::addColumn<Range>("expected");

  testNewRow() << "fe" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 2);
  testNewRow() << "fe" << Range(0, 0, 0, 8) << true << Range(0, 6, 0, 8);

  testNewRow() << "^fe" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 2);
  testNewRow() << "^fe" << Range(0, 0, 0, 1) << false << Range::invalid();
  testNewRow() << "^fe" << Range(0, 0, 0, 2) << false << Range(0, 0, 0, 2);
  testNewRow() << "^fe" << Range(0, 3, 0, 8) << false << Range::invalid(); // only match at line start
  testNewRow() << "^fe" << Range(0, 0, 0, 2) << true << Range(0, 0, 0, 2);
  testNewRow() << "^fe" << Range(0, 0, 0, 1) << true << Range::invalid();
  testNewRow() << "^fe" << Range(0, 0, 0, 2) << true << Range(0, 0, 0, 2);
  testNewRow() << "^fe" << Range(0, 3, 0, 8) << true << Range::invalid();

  testNewRow() << "fe$" << Range(0, 0, 0, 8) << false << Range(0, 6, 0, 8);
  testNewRow() << "fe$" << Range(0, 7, 0, 8) << false << Range::invalid();
  testNewRow() << "fe$" << Range(0, 6, 0, 8) << false << Range(0, 6, 0, 8);
//  testNewRow() << "fe$" << Range(0, 0, 0, 5) << false << Range::invalid(); // only match at line end, fails
  testNewRow() << "fe$" << Range(0, 0, 0, 8) << true << Range(0, 6, 0, 8);
  testNewRow() << "fe$" << Range(0, 7, 0, 8) << true << Range::invalid();
  testNewRow() << "fe$" << Range(0, 6, 0, 8) << true << Range(0, 6, 0, 8);
//  testNewRow() << "fe$" << Range(0, 0, 0, 5) << true << Range::invalid();  // fails due to $-shortcoming in QRegExp

  testNewRow() << "^fe fe fe$" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 8);
  testNewRow() << "^fe fe fe$" << Range(0, 3, 0, 8) << false << Range::invalid();
  testNewRow() << "^fe fe fe$" << Range(0, 0, 0, 5) << false << Range::invalid();
  testNewRow() << "^fe fe fe$" << Range(0, 3, 0, 5) << false << Range::invalid();
  testNewRow() << "^fe fe fe$" << Range(0, 0, 0, 8) << true << Range(0, 0, 0, 8);
  testNewRow() << "^fe fe fe$" << Range(0, 3, 0, 8) << true << Range::invalid();
  testNewRow() << "^fe fe fe$" << Range(0, 0, 0, 5) << true << Range::invalid();
  testNewRow() << "^fe fe fe$" << Range(0, 3, 0, 5) << true << Range::invalid();

  testNewRow() << "^fe( fe)*$" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 8);
  testNewRow() << "^fe( fe)*"  << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 8);
  testNewRow() <<  "fe( fe)*$" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 8);
  testNewRow() <<  "fe( fe)*"  << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 8);
  testNewRow() << "^fe( fe)*$" << Range(0, 3, 0, 8) << false << Range::invalid();
  testNewRow() <<  "fe( fe)*$" << Range(0, 3, 0, 8) << false << Range(0, 3, 0, 8);
//  testNewRow() << "^fe( fe)*$" << Range(0, 0, 0, 5) << false << Range::invalid();  // fails due to $-shortcoming in QRegExp
  testNewRow() << "^fe( fe)*"  << Range(0, 0, 0, 5) << false << Range(0, 0, 0, 5);
  testNewRow() << "^fe( fe)*$" << Range(0, 0, 0, 8) << true << Range(0, 0, 0, 8);
  testNewRow() << "^fe( fe)*"  << Range(0, 0, 0, 8) << true << Range(0, 0, 0, 8);
//  testNewRow() <<  "fe( fe)*$" << Range(0, 0, 0, 8) << true << Range(0, 0, 0, 8);  // fails, shouldn't matching be greedy?
//  testNewRow() <<  "fe( fe)*"  << Range(0, 0, 0, 8) << true << Range(0, 0, 0, 8);  // fails, shouldn't matching be greedy?
  testNewRow() << "^fe( fe)*$" << Range(0, 3, 0, 8) << true << Range::invalid();
//  testNewRow() <<  "fe( fe)*$" << Range(0, 3, 0, 8) << true << Range(0, 3, 0, 8);  // fails, shouldn't matching be greedy?
//  testNewRow() << "^fe( fe)*$" << Range(0, 0, 0, 5) << true << Range::invalid();   // fails due to $-shortcoming in QRegExp

  testNewRow() << "^fe|fe$" << Range(0, 0, 0, 5) << false << Range(0, 0, 0, 2);
  testNewRow() << "^fe|fe$" << Range(0, 3, 0, 8) << false << Range(0, 6, 0, 8);
//  testNewRow() << "^fe|fe$" << Range(0, 0, 0, 5) << true << Range(0, 0, 0, 2);  // fails due to $-shortcoming in QRegExp
  testNewRow() << "^fe|fe$" << Range(0, 3, 0, 8) << true << Range(0, 6, 0, 8);
}

void RegExpSearchTest::testAnchoredRegexp()
{
  QFETCH(QString, pattern);
  QFETCH(Range, inputRange);
  QFETCH(bool, backwards);
  QFETCH(Range, expected);

  KateDocument doc(false, false, false);
  doc.setText("fe fe fe");

  KateRegExpSearch searcher(&doc, Qt::CaseInsensitive);

  static int i = 0;
  if (i == 34 || i == 36)
    qDebug() << i;
  i++;

  const Range result = searcher.search(pattern, inputRange, backwards)[0];

  QCOMPARE(result, expected);
}

void RegExpSearchTest::testSearchForward()
{
  KateDocument doc(false, false, false);
  doc.setText("  \\piinfercong");

  KateRegExpSearch search(&doc, Qt::CaseSensitive);
  const Range result = search.search("\\\\piinfer(\\w)", Range(0, 2, 0, 15))[0];

  QCOMPARE(result, Range(0, 2, 0, 11));
}

void RegExpSearchTest::testSearchBackwardInSelection()
{
  KateDocument doc(false, false, false);
  doc.setText("foobar foo bar foo bar foo");

  KateRegExpSearch search(&doc, Qt::CaseSensitive);
  const Range result = search.search("foo", Range(0, 0, 0, 15), true)[0];

  QCOMPARE(result, Range(0, 7, 0, 10));
}

void RegExpSearchTest::test()
{
  KateDocument doc(false, false, false);
  doc.setText("\\newcommand{\\piReductionOut}");

  KateRegExpSearch search(&doc, Qt::CaseSensitive);
  const QVector<Range> result = search.search("\\\\piReduction(\\S)", Range(0, 10, 0, 28), true);

  QCOMPARE(result.size(), 2);
  QCOMPARE(result[0], Range(0, 12, 0, 25));
  QCOMPARE(result[1], Range(0, 24, 0, 25));
  QCOMPARE(doc.text(result[0]), QString("\\piReductionO"));
  QCOMPARE(doc.text(result[1]), QString("O"));
}

// kate: space-indent on; indent-width 2; replace-tabs on;
