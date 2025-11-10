/*
    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QObject>
#include <QTest>

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MovingRange>
#include <KTextEditor/View>

#include "../rainbowparens_plugin.h"

class Test : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void test();
};

static std::vector<KTextEditor::Attribute::Ptr> createAttributes()
{
    std::vector<KTextEditor::Attribute::Ptr> attrs;
    attrs.resize(numberOfColors);
    for (auto &attr : attrs) {
        attr = new KTextEditor::Attribute;
    }
    attrs[0]->setForeground(QBrush(Qt::red));
    attrs[1]->setForeground(QBrush(Qt::green));
    attrs[2]->setForeground(QBrush(Qt::yellow));
    attrs[3]->setForeground(QBrush(Qt::cyan));
    attrs[4]->setForeground(QBrush(Qt::magenta));
    return attrs;
}

void Test::test()
{
    auto editor = KTextEditor::Editor::instance();
    auto doc = editor->createDocument(nullptr);
    auto view = doc->createView(nullptr);
    const auto attributes = createAttributes();

    doc->setHighlightingMode(QStringLiteral("C++"));
    doc->setText(QString::fromLatin1(R"(int main(int, char*[1234]) { return 0; }
    )"));

    view->resize(800, 600);
    view->show();

    std::vector<std::unique_ptr<KTextEditor::MovingRange>> ranges;

    auto colorIdx = rehighlight(view, ranges, 0, attributes);
    std::ignore = colorIdx;
    QCOMPARE(colorIdx, 3);
    QCOMPARE(ranges.size(), 6);

    // ()
    QCOMPARE(ranges[0]->toRange(), KTextEditor::Range(0, 8, 0, 9));
    QCOMPARE(ranges[1]->toRange(), KTextEditor::Range(0, 25, 0, 26));
    QCOMPARE(ranges[0]->attribute(), attributes[0]);
    QCOMPARE(ranges[1]->attribute(), attributes[0]);

    // []
    QCOMPARE(ranges[2]->toRange(), KTextEditor::Range(0, 19, 0, 20));
    QCOMPARE(ranges[3]->toRange(), KTextEditor::Range(0, 24, 0, 25));
    QCOMPARE(ranges[2]->attribute(), attributes[1]);
    QCOMPARE(ranges[3]->attribute(), attributes[1]);

    // {}
    QCOMPARE(ranges[4]->toRange(), KTextEditor::Range(0, 27, 0, 28));
    QCOMPARE(ranges[5]->toRange(), KTextEditor::Range(0, 39, 0, 40));
    QCOMPARE(ranges[4]->attribute(), attributes[2]);
    QCOMPARE(ranges[5]->attribute(), attributes[2]);

    delete doc;
}

QTEST_MAIN(Test)

#include "test.moc"
