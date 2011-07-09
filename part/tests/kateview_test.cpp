/* This file is part of the KDE libraries
   Copyright (C) 2010 Milian Wolff <mail@milianw.de>

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

#include "kateview_test.h"
#include "moc_kateview_test.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <kateview.h>
#include <ktexteditor/movingcursor.h>
#include <kateconfig.h>
#include <ktemporaryfile.h>

using namespace KTextEditor;

QTEST_KDEMAIN(KateViewTest, GUI)

namespace QTest {
    template<>
    char *toString(const KTextEditor::Cursor &cursor)
    {
        QByteArray ba = "Cursor[" + QByteArray::number(cursor.line())
                           + ", " + QByteArray::number(cursor.column()) + "]";
        return qstrdup(ba.data());
    }
}


KateViewTest::KateViewTest()
  : QObject()
{
}

KateViewTest::~KateViewTest()
{
}

void KateViewTest::testReloadMultipleViews()
{
    KTemporaryFile file;
    file.setSuffix(".cpp");
    file.open();
    QTextStream stream(&file);
    const QString line = "const char* foo = \"asdf\"\n";
    for ( int i = 0; i < 200; ++i ) {
        stream << line;
    }
    stream << flush;
    file.close();

    KateDocument doc(false, false, false);
    QVERIFY(doc.openUrl(KUrl(file.fileName())));
    QCOMPARE(doc.highlightingMode(), QString("C++"));

    KateView* view1 = new KateView(&doc, 0);
    KateView* view2 = new KateView(&doc, 0);
    view1->show();
    view2->show();
    QCOMPARE(doc.views().count(), 2);

    QVERIFY(doc.documentReload());
}

void KateViewTest::testLowerCaseBlockSelection()
{
    // testcase for https://bugs.kde.org/show_bug.cgi?id=258480
    KateDocument doc(false, false, false);
    doc.setText("nY\nnYY\n");

    KateView* view1 = new KateView(&doc, 0);
    view1->setBlockSelectionMode(true);
    view1->setSelection(Range(0, 1, 1, 3));
    view1->lowercase();

    QCOMPARE(doc.text(), QString("ny\nnyy\n"));
}

void KateViewTest::testFolding_data()
{
    QTest::addColumn<bool>("dynWordWrap");
    QTest::addColumn<bool>("scrollPastEnd");
    QTest::addColumn<int>("autoCenterLines");

    QTest::newRow("dynWordWrap") << true << false << 0;
    QTest::newRow("dynWordWrap+scrollPastEnd") << true << true << 0;
    QTest::newRow("dynWordWrap+autoCenterLines") << true << false << 10;
    QTest::newRow("dynWordWrap+scrollPastEnd+autoCenterLines") << true << true << 10;
    QTest::newRow("scrollPastEnd") << false << true << 0;
    QTest::newRow("scrollPastEnd+autoCenterLines") << false << true << 10;
}

void KateViewTest::testFolding()
{
    KTemporaryFile file;
    file.setSuffix(".cpp");
    file.open();
    QTextStream stream(&file);
    stream << "int main() {\n"
           << "  asdf;\n"
           << "}\n";
    stream << flush;
    file.close();

    KateDocument doc(false, false, false);
    QVERIFY(doc.openUrl(KUrl(file.fileName())));
    QCOMPARE(doc.highlightingMode(), QString("C++"));

    KateView* view = new KateView(&doc, 0);
    QAction* collapseAction = view->action("folding_toplevel");
    QVERIFY(collapseAction);
    QAction* expandAction = view->action("folding_expandtoplevel");
    QVERIFY(expandAction);

    QFETCH(bool, dynWordWrap);
    QFETCH(bool, scrollPastEnd);
    QFETCH(int, autoCenterLines);

    view->config()->setDynWordWrap(dynWordWrap);
    view->config()->setScrollPastEnd(scrollPastEnd);
    view->config()->setAutoCenterLines(autoCenterLines);

    QCOMPARE(doc.visibleLines(), 4u);

    collapseAction->trigger();
    QCOMPARE(doc.visibleLines(), 2u);

    expandAction->trigger();
    QCOMPARE(doc.visibleLines(), 4u);
}

void KateViewTest::testSelection()
{
    // see also: https://bugs.kde.org/show_bug.cgi?id=277422
    // wrong behavior before:
    // Open file with text
    // click at end of some line (A) and drag to right, i.e. without selecting anything
    // click somewhere else (B)
    // shift click to another place (C)
    // => expected: selection from B to C
    // => actual: selection from A to C

    KTemporaryFile file;
    file.setSuffix(".txt");
    file.open();
    QTextStream stream(&file);
    stream << "A\n"
           << "B\n"
           << "C";
    stream << flush;
    file.close();

    KateDocument doc(false, false, false);
    QVERIFY(doc.openUrl(KUrl(file.fileName())));

    KateView* view = new KateView(&doc, 0);
    view->resize(100, 100);
    view->show();

    // hackish but works: access to KateViewInternal
    QObject* internalView = view->childAt(50, 50);
    QCOMPARE(internalView->metaObject()->className(), "KateViewInternal");

    const QPoint afterA = view->cursorToCoordinate(Cursor(0, 1));
    const QPoint afterB = view->cursorToCoordinate(Cursor(1, 1));
    const QPoint afterC = view->cursorToCoordinate(Cursor(2, 1));

    // click after A
    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonPress, afterA,
                                                      Qt::LeftButton, Qt::LeftButton,
                                                      Qt::NoModifier));

    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonRelease, afterA,
                                                      Qt::LeftButton, Qt::LeftButton,
                                                      Qt::NoModifier));
    QCOMPARE(view->cursorPosition(), Cursor(0, 1));
    // drag to right
    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonPress, afterA,
                                                      Qt::LeftButton, Qt::LeftButton,
                                                      Qt::NoModifier));

    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseMove, afterA + QPoint(50, 0),
                                                      Qt::LeftButton, Qt::LeftButton,
                                                      Qt::NoModifier));

    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonRelease, afterA + QPoint(50, 0),
                                                      Qt::LeftButton, Qt::LeftButton,
                                                      Qt::NoModifier));

    QCOMPARE(view->cursorPosition(), Cursor(0, 1));
    QVERIFY(!view->selection());

    // click after C
    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonPress, afterC,
                                                      Qt::LeftButton, Qt::LeftButton,
                                                      Qt::NoModifier));

    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonRelease, afterC,
                                                      Qt::LeftButton, Qt::LeftButton,
                                                      Qt::NoModifier));

    QCOMPARE(view->cursorPosition(), Cursor(2, 1));
    // shift+click after B
    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonPress, afterB,
                                                      Qt::LeftButton, Qt::LeftButton,
                                                      Qt::ShiftModifier));

    QCoreApplication::sendEvent(internalView, new QMouseEvent(QEvent::MouseButtonRelease, afterB,
                                                      Qt::LeftButton, Qt::LeftButton,
                                                      Qt::ShiftModifier));

    QCOMPARE(view->cursorPosition(), Cursor(1, 1));
    QCOMPARE(view->selectionRange(), Range(1, 1, 2, 1));
}
