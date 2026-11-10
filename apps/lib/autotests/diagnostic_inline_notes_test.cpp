/*
    SPDX-FileCopyrightText: 2026 Jaume Alcazo Castellarnau <jaume.alcazo@gmail.com>

    SPDX-License-Identifier: MIT
*/
#include "diagnostics/diagnosticinlinenotes.h"

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MovingRange>

#include <QFontMetricsF>
#include <QGuiApplication>
#include <QTest>

#include <memory>

class DiagnosticInlineNotesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void noteAtEndOfLine();
    void noteFollowsEdits();
    void mostSevereFirstAndCountKept();
    void emptyMessageIgnored();

private:
    struct Fixture {
        std::unique_ptr<KTextEditor::Document> doc;
        std::vector<std::unique_ptr<KTextEditor::MovingRange>> ranges;
        std::unique_ptr<DiagnosticInlineNoteProvider> notes;

        explicit Fixture(const QString &text)
            : doc(KTextEditor::Editor::instance()->createDocument(nullptr))
        {
            doc->setText(text);
            notes = std::make_unique<DiagnosticInlineNoteProvider>(doc.get());
        }
        ~Fixture()
        {
            // the provider must go before the ranges it points to
            notes.reset();
            ranges.clear();
        }
        void add(KTextEditor::Range r, const QString &message, DiagnosticSeverity severity)
        {
            ranges.emplace_back(doc->newMovingRange(r));
            notes->addDiagnostic(ranges.back().get(), message, severity);
        }
    };
};

QTEST_MAIN(DiagnosticInlineNotesTest)

void DiagnosticInlineNotesTest::noteAtEndOfLine()
{
    Fixture f(QStringLiteral("int a;\nint b = x;\n"));
    f.add({1, 8, 1, 9}, QStringLiteral("use of undeclared identifier 'x'"), DiagnosticSeverity::Error);

    QCOMPARE(f.notes->inlineNotes(1), QList<int>{f.doc->lineLength(1)});
    QVERIFY(f.notes->inlineNotes(0).isEmpty());
    QVERIFY(f.notes->inlineNotes(2).isEmpty());
}

void DiagnosticInlineNotesTest::noteFollowsEdits()
{
    Fixture f(QStringLiteral("int a;\nint b = x;\n"));
    f.add({1, 8, 1, 9}, QStringLiteral("use of undeclared identifier 'x'"), DiagnosticSeverity::Error);

    // a new line above moves the diagnostic, and its note, one line down
    f.doc->insertText({0, 0}, QStringLiteral("// comment\n"));
    QVERIFY(f.notes->inlineNotes(1).isEmpty());
    QCOMPARE(f.notes->inlineNotes(2), QList<int>{f.doc->lineLength(2)});
}

void DiagnosticInlineNotesTest::mostSevereFirstAndCountKept()
{
    Fixture f(QStringLiteral("int a = b + c;\n"));
    f.add({0, 8, 0, 9}, QStringLiteral("a warning"), DiagnosticSeverity::Warning);
    f.add({0, 12, 0, 13}, QStringLiteral("the error\nsecond line is dropped"), DiagnosticSeverity::Error);

    const auto note = f.notes->noteForLine(0);
    QVERIFY(note.entry);
    QCOMPARE(note.count, 2);
    QCOMPARE(note.entry->message, QStringLiteral("the error"));

    const QFontMetricsF fm(QGuiApplication::font());
    QCOMPARE(f.notes->noteText(note, fm, 10000.), QStringLiteral("the error (+1)"));
    // even when there is no room, the message is elided but the count stays
    const auto narrow = f.notes->noteText(note, fm, 1.);
    QVERIFY(narrow.endsWith(QStringLiteral(" (+1)")));
}

void DiagnosticInlineNotesTest::emptyMessageIgnored()
{
    Fixture f(QStringLiteral("int a;\n"));
    f.add({0, 0, 0, 3}, QStringLiteral("\n  \n"), DiagnosticSeverity::Error);
    QVERIFY(f.notes->inlineNotes(0).isEmpty());
}

#include "diagnostic_inline_notes_test.moc"
