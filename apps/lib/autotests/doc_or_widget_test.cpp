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

    dw = DocOrWidget::null();
    QVERIFY(dw.isNull());
    QVERIFY(!dw.qobject());
    QVERIFY(!dw.widget());
    QVERIFY(!dw.doc());
}

void DocOrWidgetTest::testDocOrWidget()
{
    DocOrWidget dw = DocOrWidget::null();
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
}

QTEST_MAIN(DocOrWidgetTest)

#include "doc_or_widget_test.moc"
