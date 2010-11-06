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

#include "katedocument_test.h"
#include "moc_katedocument_test.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <ktexteditor/movingcursor.h>
#include <kateconfig.h>
#include <ktemporaryfile.h>

///TODO: is there a FindValgrind cmake command we could use to
///      define this automatically?
// comment this out and run the test case with:
//   valgrind --tool=callgrind --instr-atstart=no ./katedocument_test testSetTextPerformance
// or similar
//
// #define USE_VALGRIND

#ifdef USE_VALGRIND
  #include <valgrind/callgrind.h>
#endif

using namespace KTextEditor;

QTEST_KDEMAIN(KateDocumentTest, GUI)

namespace QTest {
    template<>
    char *toString(const KTextEditor::Cursor &cursor)
    {
        QByteArray ba = "Cursor[" + QByteArray::number(cursor.line())
                           + ", " + QByteArray::number(cursor.column()) + "]";
        return qstrdup(ba.data());
    }
}


KateDocumentTest::KateDocumentTest()
  : QObject()
{
}

KateDocumentTest::~KateDocumentTest()
{
}

// tests:
// KateDocument::insertText with word wrap enabled. It is checked whether the
// text is correctly wrapped and whether the moving cursors maintain the correct
// position.
// see also: http://bugs.kde.org/show_bug.cgi?id=168534
void KateDocumentTest::testWordWrap()
{
    KateDocument doc (false, false, false);
    doc.setWordWrap(true);
    doc.setWordWrapAt(80);

    const QString content = ".........1.........2.........3.........4.........5.........6 ........7 ........8";
    const QString firstWrap = ".........1.........2.........3.........4.........5.........6 ........7\n....x....8";
    const QString secondWrap = ".........1.........2.........3.........4.........5.........6\n....ooooooooooo....7 ....x....8";
    doc.setText(content);
    MovingCursor* c = doc.newMovingCursor(Cursor(0, 75), MovingCursor::MoveOnInsert);

    QCOMPARE(doc.text(), content);
    QCOMPARE(c->toCursor(), Cursor(0, 75));

    // type a character at (0, 75)
    doc.insertText (c->toCursor(), "x");
    QCOMPARE(doc.text(), firstWrap);
    QCOMPARE(c->toCursor(), Cursor(1, 5));

    // set cursor to (0, 65) and type "ooooooooooo"
    c->setPosition(Cursor(0, 65));
    doc.insertText (c->toCursor(), "ooooooooooo");
    QCOMPARE(doc.text(), secondWrap);
    QCOMPARE(c->toCursor(), Cursor(1, 15));
}

void KateDocumentTest::testReplaceQStringList()
{
    KateDocument doc(false, false, false);
    doc.setWordWrap(false);
    doc.setText("asdf\n"
                "foo\n"
                "foo\n"
                "bar\n");
    doc.replaceText( Range(1, 0, 3, 0), QStringList() << "new" << "text" << "", false );
    QCOMPARE(doc.text(), QString("asdf\n"
                                 "new\n"
                                 "text\n"
                                 "bar\n"));
}

void KateDocumentTest::testRemoveTrailingSpace()
{
    // https://bugs.kde.org/show_bug.cgi?id=242611
    KateDocument doc(false, false, false);
    doc.setText("asdf         \t   ");
    doc.config()->setRemoveTrailingDyn(true);
    doc.editRemoveText(0, 0, 1);
    QCOMPARE(doc.text(), QString("sdf"));
}

void KateDocumentTest::testMovingInterfaceSignals()
{
    KateDocument* doc = new KateDocument(false, false, false);
    QSignalSpy aboutToDeleteSpy(doc, SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document *)));
    QSignalSpy aboutToInvalidateSpy(doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document *)));

    QCOMPARE(doc->revision(), qint64(0));

    QCOMPARE(aboutToInvalidateSpy.count(), 0);
    QCOMPARE(aboutToDeleteSpy.count(), 0);

    KTemporaryFile f;
    f.open();
    doc->openUrl(KUrl::fromLocalFile(f.fileName()));
    QCOMPARE(doc->revision(), qint64(0));
    //TODO: gets emitted once in closeFile and once in openFile - is that OK?
    QCOMPARE(aboutToInvalidateSpy.count(), 2);
    QCOMPARE(aboutToDeleteSpy.count(), 0);

    doc->documentReload();
    QCOMPARE(doc->revision(), qint64(0));
    QCOMPARE(aboutToInvalidateSpy.count(), 4);
    //TODO: gets emitted once in closeFile and once in openFile - is that OK?
    QCOMPARE(aboutToDeleteSpy.count(), 0);

    delete doc;
    QCOMPARE(aboutToInvalidateSpy.count(), 4);
    QCOMPARE(aboutToDeleteSpy.count(), 1);
}

void KateDocumentTest::testSetTextPerformance()
{
    const int lines = 150;
    const int columns = 80;
    const int rangeLength = 4;
    const int rangeGap = 1;

    Q_ASSERT(columns % (rangeLength + rangeGap) == 0);

    KateDocument doc(false, false, false);
    QString text;
    QVector<Range> ranges;
    ranges.reserve(lines * columns / (rangeLength + rangeGap));
    const QString line = QString().fill('a', columns);
    for(int l = 0; l < lines; ++l) {
        text.append(line);
        text.append('\n');
        for(int c = 0; c < columns; c += rangeLength + rangeGap) {
            ranges << Range(l, c, l, c + rangeLength);
        }
    }
    // init
    doc.setText(text);
    QVector<MovingRange*> movingRanges;
    movingRanges.reserve(ranges.size());
    foreach(const Range& range, ranges) {
        movingRanges << doc.newMovingRange(range);
    }

    #ifdef USE_VALGRIND
        CALLGRIND_START_INSTRUMENTATION
    #endif

    // replace
    QBENCHMARK {
        doc.setText(text);
    }

    #ifdef USE_VALGRIND
        CALLGRIND_STOP_INSTRUMENTATION
    #endif
}
