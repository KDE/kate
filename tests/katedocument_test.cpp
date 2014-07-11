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

class MovingRangeInvalidator : public QObject {
    Q_OBJECT
public:
  explicit MovingRangeInvalidator( QObject* parent = 0 )
    : QObject(parent)
  {
  }

  void addRange(MovingRange* range)
  {
    m_ranges << range;
  }
  QList<MovingRange*> ranges() const
  {
    return m_ranges;
  }

public slots:
    void aboutToInvalidateMovingInterfaceContent()
    {
        qDeleteAll(m_ranges);
        m_ranges.clear();
    }

private:
    QList<MovingRange*> m_ranges;
};


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
    
    // space after 7 is now kept
    // else we kill indentation...
    const QString firstWrap = ".........1.........2.........3.........4.........5.........6 ........7 \n....x....8";
    
    // space after 6 is now kept
    // else we kill indentation...
    const QString secondWrap = ".........1.........2.........3.........4.........5.........6 \n....ooooooooooo....7 ....x....8";
    
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

void KateDocumentTest::testMovingInterfaceSignals()
{
    KateDocument* doc = new KateDocument(false, false, false);
    QSignalSpy aboutToDeleteSpy(doc, SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document*)));
    QSignalSpy aboutToInvalidateSpy(doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)));

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
    MovingRangeInvalidator invalidator;
    connect(&doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
            &invalidator, SLOT(aboutToInvalidateMovingInterfaceContent()));

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

    // replace
    QBENCHMARK {
        // init
        doc.setText(text);
        foreach(const Range& range, ranges) {
            invalidator.addRange(doc.newMovingRange(range));
        }
        QCOMPARE(invalidator.ranges().size(), ranges.size());

        #ifdef USE_VALGRIND
            CALLGRIND_START_INSTRUMENTATION
        #endif

        doc.setText(text);

        #ifdef USE_VALGRIND
            CALLGRIND_STOP_INSTRUMENTATION
        #endif

        QCOMPARE(doc.text(), text);
        QVERIFY(invalidator.ranges().isEmpty());
    }
}

void KateDocumentTest::testRemoveTextPerformance()
{
    const int lines = 5000;
    const int columns = 80;

    KateDocument doc(false, false, false);

    QString text;
    const QString line = QString().fill('a', columns);
    for(int l = 0; l < lines; ++l) {
        text.append(line);
        text.append('\n');
    }

    doc.setText(text);

    // replace
    QBENCHMARK_ONCE {
        #ifdef USE_VALGRIND
            CALLGRIND_START_INSTRUMENTATION
        #endif

        doc.editStart();

        doc.removeText(doc.documentRange());

        doc.editEnd();

        #ifdef USE_VALGRIND
            CALLGRIND_STOP_INSTRUMENTATION
        #endif
    }
}

void KateDocumentTest::testForgivingApiUsage()
{
    KateDocument doc(false, false, false);

    QVERIFY(doc.isEmpty());
    QVERIFY(doc.replaceText(Range(0, 0, 100, 100), "asdf"));
    QCOMPARE(doc.text(), QString("asdf"));
    QCOMPARE(doc.lines(), 1);
    QVERIFY(doc.replaceText(Range(2, 99, 2, 100), "asdf"));
    QEXPECT_FAIL("", "replacing text behind the document end will add 5 lines out of nowhere", Continue);
    QCOMPARE(doc.lines(), 3);

    QVERIFY(doc.removeText(Range(0, 0, 1000, 1000)));
    QVERIFY(doc.removeText(Range(0, 0, 0, 100)));
    QVERIFY(doc.isEmpty());
    doc.insertText(Cursor(100, 0), "foobar");
    QCOMPARE(doc.line(100), QString("foobar"));

    doc.setText("nY\nnYY\n");
    QVERIFY(doc.removeText(Range(0, 0, 0, 1000)));
}

/**
 * Provides slots to check data sent in specific signals. Slot names are derived from corresponding test names.
 */
class SignalHandler : public QObject
{
    Q_OBJECT
public slots:
    void slotMultipleLinesRemoved(KTextEditor::Document*, const KTextEditor::Range&, const QString& oldText)
    {
        QCOMPARE(oldText, QString("line2\nline3\n"));
    }

    void slotNewlineInserted(KTextEditor::Document*, const KTextEditor::Range& range)
    {
        QCOMPARE(range, Range(Cursor(1, 4), Cursor(2, 0)));
    }
};

void KateDocumentTest::testRemoveMultipleLines()
{
   KateDocument doc(false, false, false);

    doc.setText("line1\n"
        "line2\n"
        "line3\n"
        "line4\n");

    SignalHandler handler;
    connect(&doc, SIGNAL(textRemoved(KTextEditor::Document*,KTextEditor::Range,QString)), &handler, SLOT(slotMultipleLinesRemoved(KTextEditor::Document*,KTextEditor::Range,QString)));
    doc.removeText(Range(1, 0, 3, 0));
}

void KateDocumentTest::testInsertNewline()
{
    KateDocument doc(false, false, false);

    doc.setText("this is line\n"
        "this is line2\n");

    SignalHandler handler;
    connect(&doc, SIGNAL(textInserted(KTextEditor::Document*,KTextEditor::Range)), &handler, SLOT(slotNewlineInserted(KTextEditor::Document*,KTextEditor::Range)));
    doc.editWrapLine(1, 4);
}

// we have two different ways of creating the md5 checksum:
// in KateFileLoader and KateDocument::createDigest. Make
// sure, these two implementations result in the same checksum.
void KateDocumentTest::testDigest()
{
  // md5sum of data/md5checksum.txt: ff6e0fddece03adeb8f902e8c540735a
  // QCryptographicHash is used, therefore we need fromHex here
  const QByteArray fileDigest = QByteArray::fromHex("ff6e0fddece03adeb8f902e8c540735a");

  // make sure, Kate::TextBuffer and KateDocument::createDigest() equal
  KateDocument doc(false, false, false);
  doc.openUrl(QString(KDESRCDIR + QString("data/md5checksum.txt")));
  const QByteArray bufferDigest(doc.digest());
  QVERIFY(doc.createDigest());
  const QByteArray docDigest(doc.digest());

  QCOMPARE(bufferDigest, fileDigest);
  QCOMPARE(docDigest, fileDigest);
}

void KateDocumentTest::testDefStyleNum()
{
  KateDocument doc;
  doc.setText("foo\nbar\nasdf");
  QCOMPARE(doc.defStyleNum(0, 0), -1);
}

#include "katedocument_test.moc"
