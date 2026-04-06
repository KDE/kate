#include "diffwidget_tests.h"
#include "diffwidget.h"

#include <QScrollBar>
#include <QTest>

QTEST_MAIN(DiffWidgetTests)

bool operator==(DiffWidget::ChangePair l, DiffWidget::ChangePair r)
{
    return l.left.pos == r.left.pos && l.left.len == r.left.len && l.right.pos == r.right.pos && l.right.len == r.right.len;
}

namespace QTest
{
template<>
char *toString(const DiffWidget::ChangePair &p)
{
    char buf[256]{};
    int out = std::snprintf(buf, 255, "{{%lld -> %lld} -- {%lld -> %lld}}", p.left.pos, p.left.len, p.right.pos, p.right.len);
    buf[out] = 0;
    return qstrdup(buf);
}
}

void DiffWidgetTests::test_scrollbarAtTopOnOpen()
{
    QFile f(QStringLiteral(":/katetest/test.diff"));
    QVERIFY(f.open(QFile::ReadOnly));
    const QByteArray diff = f.readAll();

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

void DiffWidgetTests::test_inlineDiff_data()
{
    QTest::addColumn<QString>("left");
    QTest::addColumn<QString>("right");
    QTest::addColumn<DiffWidget::ChangePair>("expected");

    using ChangePair = DiffWidget::ChangePair;

    QTest::addRow("1") << QStringLiteral("asdCat") << QStringLiteral("xzCat") << ChangePair{.left = {.pos = 0, .len = 3}, .right = {.pos = 0, .len = 2}};
    QTest::addRow("2") << QStringLiteral("hello") << QStringLiteral("hello") << ChangePair{.left = {.pos = 5, .len = 0}, .right = {.pos = 5, .len = 0}};
    QTest::addRow("3") << QStringLiteral("") << QStringLiteral("") << ChangePair{};
    QTest::addRow("4") << QStringLiteral("c") << QStringLiteral("cat") << ChangePair{.left = {.pos = 1, .len = 0}, .right = {.pos = 1, .len = 2}};
    QTest::addRow("5") << QStringLiteral("a b c") << QStringLiteral("a d e") << ChangePair{.left = {.pos = 2, .len = 3}, .right = {.pos = 2, .len = 3}};
    QTest::addRow("6") << QString() << QString() << ChangePair{};
}

void DiffWidgetTests::test_inlineDiff()
{
    QFETCH(QString, left);
    QFETCH(QString, right);
    QFETCH(DiffWidget::ChangePair, expected);

    QCOMPARE(DiffWidget::inlineDiff(left, right), expected);
    // qDebug() << result.left.pos << result.left.len << result.right.pos << result.right.len;
}

#include "moc_diffwidget_tests.cpp"
