/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "test_formatapply.h"

#include "FormatApply.h"
#include "FormattersEnum.h"
#include <KTextEditor/Editor>

#include <QTest>

QTEST_MAIN(FormatApplyTest)

void FormatApplyTest::testFormatApply()
{
    const char patch[] =
        R"(diff --git a/home/waqar/kde/src/utilities/kate/build/m.cpp b/tmp/kate.ypQGnu
index dc6e4c4df..03f3d79d9 100644
--- a/home/waqar/kde/src/utilities/kate/build/m.cpp
+++ b/tmp/kate.ypQGnu
@@ -1,4 +1,5 @@
 #include <cstdio>
-int main(){
+int main()
+{
     puts("hello");
 }
)";

    auto editor = KTextEditor::Editor::instance();
    auto doc = editor->createDocument(this);
    const char unformatted[] =
        R"(#include <cstdio>
int main(){
    puts("hello");
})";

    doc->setText(QString::fromUtf8(unformatted));
    const auto edits = parseDiff(doc, QString::fromUtf8(patch));
    QVERIFY(!edits.empty());

    applyPatch(doc, edits);

    const QString formatted = QStringLiteral(
        R"(#include <cstdio>
int main()
{
    puts("hello");
})");
    QCOMPARE(doc->text(), formatted);
}

void FormatApplyTest::testFormatterForName()
{
    auto fmtToStr = [](Formatters f) {
        switch (f) {
        case Formatters::ClangFormat:
            return "clang-format";
        case Formatters::Prettier:
            return "prettier";
        case Formatters::Jq:
            return "jq";
        case Formatters::XmlLint:
            return "xmllint";
        case Formatters::Autopep8:
            return "autopep8";
        case Formatters::Ruff:
            return "ruff";
        case Formatters::YamlFmt:
            return "yamlfmt";
        case Formatters::COUNT:
            Q_ASSERT(false);
            return "";
        }
        return "";
    };

    for (int i = (int)Formatters::FIRST; i < (int)Formatters::COUNT; ++i) {
        auto fmt = fmtToStr((Formatters)i);
        QCOMPARE(formatterForName(QString::fromLatin1(fmt), Formatters::COUNT), (Formatters)i);
    }
}

#include "moc_test_formatapply.cpp"
