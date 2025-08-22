/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "hostprocess.h"
#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/MovingCursor>
#include <QFileInfo>
#include <QIcon>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <gitprocess.h>
#include <ktexteditor_utils.h>

[[maybe_unused]] static QString diff(KTextEditor::Document *doc, const QByteArray &formatted)
{
    QTemporaryFile f;
    if (!f.open()) {
        Utils::showMessage(i18n("Failed to write a temp file"), {}, i18n("Format"), MessageType::Warning);
        return {};
    }
    f.write(formatted);
    f.close();

    QProcess p;
    QStringList args = {QStringLiteral("diff"), QStringLiteral("--no-color"), QStringLiteral("--no-index")};
    args << doc->url().toString(QUrl::PreferLocalFile);
    args << f.fileName();
    if (!setupGitProcess(p, QFileInfo(doc->url().toString(QUrl::PreferLocalFile)).absolutePath(), args)) {
        Utils::showMessage(i18n("Failed to run git diff: git not installed"), {}, i18n("Format"), MessageType::Warning);
        return {};
    }
    startHostProcess(p);
    if (!p.waitForStarted() || !p.waitForFinished()) {
        Utils::showMessage(i18n("Failed to run git diff: %1", p.errorString()), {}, i18n("Format"), MessageType::Warning);
        return {};
    }

    return QString::fromUtf8(p.readAllStandardOutput());
}

struct PatchLine {
    KTextEditor::MovingCursor *pos = nullptr;
    KTextEditor::Cursor inPos;
    enum {
        Remove,
        Add
    } type;
    QString text;
};

struct DiffRange {
    uint line{};
    uint count{};
};

[[maybe_unused]] static DiffRange parseRange(const QString &range)
{
    int commaPos = range.indexOf(QLatin1Char(','));
    if (commaPos > -1) {
        return {QStringView(range).sliced(0, commaPos).toUInt(), QStringView(range).sliced(commaPos + 1).toUInt()};
    }
    return {range.toUInt(), 1};
}

[[maybe_unused]] static std::vector<PatchLine> parseDiff(KTextEditor::Document *doc, const QString &diff)
{
    static const QRegularExpression HUNK_HEADER_RE(QStringLiteral("^@@ -([0-9,]+) \\+([0-9,]+) @@(.*)"));

    std::vector<PatchLine> lines;
    const QStringList d = diff.split(QStringLiteral("\n"));
    for (int i = 0; i < d.size(); ++i) {
        const QString &l = d.at(i);
        const QRegularExpressionMatch match = HUNK_HEADER_RE.match(l);
        if (!match.hasMatch()) {
            continue;
        }

        const DiffRange src = parseRange(match.captured(1));
        const DiffRange tgt = parseRange(match.captured(2));

        // unroll into the hunk
        int srcline = src.line - 1;
        int tgtline = tgt.line - 1;
        // qDebug() << "NEW HUNK: " << l << "------------" << srcline << tgtline;
        for (int j = i + 1; j < d.size(); ++j) {
            const QString &hl = d.at(j);
            if (hl.startsWith(QLatin1Char(' '))) {
                srcline++;
                tgtline++;
            } else if (hl.startsWith(QLatin1Char('+'))) {
                PatchLine p;
                p.type = PatchLine::Add;
                p.text = hl.mid(1);
                p.inPos = KTextEditor::Cursor(tgtline, 0);
                // p.pos = iface->newMovingCursor(KTextEditor::Cursor(tgtline, 0));
                lines.push_back(p);
                // qDebug() << "insert line" << tgtline << p.text << p.inPos.line();
                tgtline++;
            } else if (hl.startsWith(QLatin1Char('-'))) {
                PatchLine p;
                p.type = PatchLine::Remove;
                p.pos = doc->newMovingCursor(KTextEditor::Cursor(srcline, 0));
                // qDebug() << "remove line" << srcline << hl.mid(1) << p.pos->line();
                lines.push_back(p);
                srcline++;
            } else if (hl.startsWith(QStringLiteral("@@ "))) {
                i = j - 1; // advance i to next hunk
                break;
            }
        }
    }
    // qDebug("================");

    return lines;
}

[[maybe_unused]] static void applyPatch(KTextEditor::Document *doc, const std::vector<PatchLine> &edits)
{
    // EditingTransaction scope
    KTextEditor::Document::EditingTransaction t(doc);
    for (const PatchLine &p : edits) {
        if (p.type == PatchLine::Add) {
            // qDebug() << "insert at " << p.inPos.line() << "text: " << p.text;
            doc->insertLine(p.inPos.line(), p.text /*+ QStringLiteral("\n")*/);
        } else if (p.type == PatchLine::Remove) {
            // qDebug() << "remove line" << p.pos->line() << doc->line(p.pos->line());
            doc->removeLine(p.pos->line());
        }
    }
    for (const PatchLine &p : edits) {
        delete p.pos;
    }
}
