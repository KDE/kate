/*
 * SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "doc_or_widget.h"
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <QTest>
#include <QWidget>

class DocOrWidgetTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void nullTest();
    void testDocOrWidget();
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

QTEST_MAIN(DocOrWidgetTest)

#include "doc_or_widget_test.moc"
