/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "test_formatapply.h"

#include "FormatApply.h"
#include "Formatters.h"
#include "FormattersEnum.h"
#include <KTextEditor/Editor>

#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QSignalSpy>
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
    const std::vector<PatchLine> edits = parseDiff(doc, QString::fromUtf8(patch));
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

// A formatter "fake" running the fake_formatter binary with the given arguments
static FormatterRunner *fakeFormatter(KTextEditor::Document *doc, const QStringList &args)
{
    QJsonArray command{QStringLiteral(FAKE_FORMATTER)};
    for (const auto &arg : args) {
        command.append(arg);
    }
    const QJsonObject config{{QStringLiteral("fake"), QJsonObject{{QStringLiteral("command"), command}}}};
    return new FormatterRunner(Formatter{.name = QStringLiteral("fake"), .args = {}, .supportsStdin = true}, config, doc);
}

void FormatApplyTest::testFailedFormatterOutputIsNotApplied_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::newRow("crash") << QStringList{QStringLiteral("--lines"), QStringLiteral("2"), QStringLiteral("--crash")};
    QTest::newRow("exit code") << QStringList{QStringLiteral("--lines"), QStringLiteral("2"), QStringLiteral("--exit"), QStringLiteral("3")};
}

// The output of a formatter that crashed or failed is incomplete, it must not be applied,
// else the rest of the document gets deleted. Such formatters might print nothing on stderr.
void FormatApplyTest::testFailedFormatterOutputIsNotApplied()
{
    QFETCH(QStringList, args);

    auto doc = KTextEditor::Editor::instance()->createDocument(this);
    doc->setText(QStringLiteral("line1\nline2\nline3\nline4\n"));

    QPointer<FormatterRunner> runner = fakeFormatter(doc, args);
    QSignalSpy formatted(runner.data(), &FormatterRunner::textFormatted);
    QSignalSpy message(runner.data(), &FormatterRunner::message);
    runner->run(doc);

    // the runner deletes itself once the formatter finished
    QTRY_VERIFY_WITH_TIMEOUT(!runner, 5000);
    QCOMPARE(formatted.count(), 0);
    QVERIFY(message.count() > 0);
    delete doc;
}

#include "moc_test_formatapply.cpp"
