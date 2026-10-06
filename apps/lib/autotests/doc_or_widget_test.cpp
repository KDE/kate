/*
 * SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "doc_or_widget.h"
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <QSet>
#include <QTest>
#include <QWidget>
#include <unordered_set>

class DocOrWidgetTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void nullTest();
    void testDocOrWidget();
    void testHash();
};

void DocOrWidgetTest::nullTest()
{
    KTextEditor::Document *document = nullptr;
    DocOrWidget dw = document;
    QVERIFY(dw.isNull());
    QVERIFY(!dw.qobject());
    QVERIFY(!dw.widget());
    QVERIFY(!dw.doc());

    dw = static_cast<QWidget *>(nullptr);
    QVERIFY(dw.isNull());
    QVERIFY(!dw.qobject());
    QVERIFY(!dw.widget());
    QVERIFY(!dw.doc());

    dw = DocOrWidget();
    QVERIFY(dw.isNull());
    QVERIFY(!dw.qobject());
    QVERIFY(!dw.widget());
    QVERIFY(!dw.doc());
}

void DocOrWidgetTest::testDocOrWidget()
{
    DocOrWidget dw;
    QVERIFY(dw.isNull());
    QVERIFY(!dw.qobject());

    KTextEditor::Document *document = KTextEditor::Editor::instance()->createDocument(nullptr);
    dw = document;
    QVERIFY(!dw.isNull());
    QVERIFY(dw.qobject());
    QVERIFY(!dw.widget());
    QVERIFY(dw.doc());
    QList<DocOrWidget> docs{dw};
    docs.removeAll(dw);
    QVERIFY(docs.isEmpty());
    delete document;

    QWidget *w = new QWidget();
    dw = w;
    QVERIFY(!dw.isNull());
    QVERIFY(dw.qobject());
    QVERIFY(dw.widget());
    QVERIFY(!dw.doc());
    delete w;

    // Construction
    static_assert(DocOrWidget(static_cast<KTextEditor::Document *>(nullptr)).m_type == DocOrWidget::Type::Document);
    static_assert(DocOrWidget(static_cast<QWidget *>(nullptr)).m_type == DocOrWidget::Type::Widget);
    static_assert(DocOrWidget().m_type == DocOrWidget::Type::None);

    // Nullness
    static_assert(DocOrWidget().isNull());
    static_assert(DocOrWidget() == static_cast<KTextEditor::Document *>(nullptr));
    static_assert(DocOrWidget() == static_cast<QWidget *>(nullptr));
    static_assert(DocOrWidget(static_cast<KTextEditor::Document *>(nullptr)).isNull());
    static_assert(DocOrWidget(static_cast<QWidget *>(nullptr)).isNull());

    // Assignment
    static_assert((DocOrWidget() = static_cast<QWidget *>(nullptr)).m_type == DocOrWidget::Type::Widget);
    static_assert((DocOrWidget() = static_cast<KTextEditor::Document *>(nullptr)).m_type == DocOrWidget::Type::Document);
    static_assert((DocOrWidget(static_cast<QWidget *>(nullptr)) = DocOrWidget()).m_type == DocOrWidget::Type::None);
    static_assert((DocOrWidget(static_cast<KTextEditor::Document *>(nullptr)) = DocOrWidget()).m_type == DocOrWidget::Type::None);
}

void DocOrWidgetTest::testHash()
{
    DocOrWidget none;
    DocOrWidget nullDoc = static_cast<KTextEditor::Document *>(nullptr);
    DocOrWidget nullWidget = static_cast<QWidget *>(nullptr);

    // Verify qHash works and is callable
    QCOMPARE(qHash(none), qHash(DocOrWidget()));
    QCOMPARE(qHash(nullDoc), qHash(DocOrWidget(static_cast<KTextEditor::Document *>(nullptr))));
    QCOMPARE(qHash(nullWidget), qHash(DocOrWidget(static_cast<QWidget *>(nullptr))));

    // Verify std::hash works and delegates to qHash
    std::hash<DocOrWidget> stdHasher;
    QCOMPARE(stdHasher(none), qHash(none));
    QCOMPARE(stdHasher(nullDoc), qHash(nullDoc));

    // Test with QSet and std::unordered_set
    QSet<DocOrWidget> qset;
    qset.insert(none);
    qset.insert(nullDoc);
    qset.insert(nullWidget);
    QCOMPARE(qset.size(), 3);
    QVERIFY(qset.contains(none));
    QVERIFY(qset.contains(nullDoc));
    QVERIFY(qset.contains(nullWidget));

    std::unordered_set<DocOrWidget> stdSet;
    stdSet.insert(none);
    stdSet.insert(nullDoc);
    stdSet.insert(nullWidget);
    QCOMPARE(stdSet.size(), 3);
    QVERIFY(stdSet.contains(none));
    QVERIFY(stdSet.contains(nullDoc));
    QVERIFY(stdSet.contains(nullWidget));

    // With real instances
    KTextEditor::Document *document = KTextEditor::Editor::instance()->createDocument(nullptr);
    QWidget *w = new QWidget();
    DocOrWidget docObj = document;
    DocOrWidget widgetObj = w;

    qset.insert(docObj);
    qset.insert(widgetObj);
    QCOMPARE(qset.size(), 5);
    QVERIFY(qset.contains(docObj));
    QVERIFY(qset.contains(widgetObj));

    delete document;
    delete w;
}

QTEST_MAIN(DocOrWidgetTest)

#include "doc_or_widget_test.moc"
