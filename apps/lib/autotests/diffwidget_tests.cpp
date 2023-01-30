#include "diffwidget_tests.h"
#include "diffwidget.h"

#include <QScrollBar>
#include <QTest>

QTEST_MAIN(DiffWidgetTests)

void DiffWidgetTests::test_scrollbarAtTopOnOpen()
{
    QFile f(QStringLiteral(":/kxmlgui5/kate/data/test.diff"));
    QVERIFY(f.open(QFile::ReadOnly));
    const auto diff = f.readAll();

    DiffWidget dw(DiffParams{});
    dw.show();
    dw.m_style = SideBySide;

    dw.openDiff(diff);

    qApp->processEvents();

    // Test not empty
    QVERIFY(!dw.m_left->document()->isEmpty());
    QVERIFY(!dw.m_right->document()->isEmpty());

    // Scrollbar should be at the top
    QCOMPARE(dw.m_left->verticalScrollBar()->value(), 0);
    QCOMPARE(dw.m_right->verticalScrollBar()->value(), 0);

    // Clear
    dw.clearData();
    // After clear both should be empty
    QVERIFY(dw.m_left->document()->isEmpty());
    QVERIFY(dw.m_right->document()->isEmpty());

    // Change to unified
    dw.m_style = Unified;
    dw.openDiff(diff);
    qApp->processEvents();

    QVERIFY(!dw.m_left->document()->isEmpty());
    QVERIFY(dw.m_right->document()->isEmpty()); // In Unified, right is empty

    QCOMPARE(dw.m_left->verticalScrollBar()->value(), 0);
}
