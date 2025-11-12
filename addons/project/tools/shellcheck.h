/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2018 Gregor Mi <codestruct@posteo.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "../kateprojectcodeanalysistool.h"
#include "kateproject.h"
#include <KLocalizedString>

/**
 * Information provider for shellcheck
 */
class KateProjectCodeAnalysisToolShellcheck : public KateProjectCodeAnalysisTool
{
public:
    using KateProjectCodeAnalysisTool::KateProjectCodeAnalysisTool;

    QString name() const override
    {
        return i18n("ShellCheck (sh/bash)");
    }

    QString description() const override
    {
        return i18n("ShellCheck is a static analysis and linting tool for sh/bash scripts");
    }

    QString fileExtensions() const override
    {
        // TODO: How to also handle files with no file extension?
        return QStringLiteral("sh|bash");
    }

    QStringList filter(const QStringList &files) const override
    {
        // for now we expect files with extension
        return files.filter(QRegularExpression(QStringLiteral("\\.(") + fileExtensions() + QStringLiteral(")$")));
    }

    QString path() const override
    {
        return QStringLiteral("shellcheck");
    }

    QStringList arguments() override
    {
        QStringList _args;

        // shellcheck --format=gcc script.sh
        // Example output:
        //        script.sh:2:12: note: Use ./*glob* or -- *glob* so names with dashes won't become options. [SC2035]
        //        script.sh:3:11: note: Use ./*glob* or -- *glob* so names with dashes won't become options. [SC2035]
        //        script.sh:3:20: warning: podir is referenced but not assigned. [SC2154]
        //        script.sh:3:20: note: Double quote to prevent globbing and word splitting. [SC2086]

        _args << QStringLiteral("--format=gcc");

        if (m_project) {
            auto &&fileList = filter(m_project->files());
            setActualFilesCount(fileList.size());
            _args.append(fileList);
        }

        return _args;
    }

    QString notInstalledMessage() const override
    {
        return i18n("Please install ShellCheck (see https://www.shellcheck.net).");
    }

    FileDiagnostics parseLine(const QString &line) const override
    {
        // Example:
        // IN:
        // script.sh:3:11: note: Use ./*glob* or -- *glob* so names with dashes won't become options. [SC2035]
        // OUT:
        // file, line, severity, message
        // "script.sh", "3", "note", "... ..."

        static const QRegularExpression regex(QStringLiteral("([^:]+):(\\d+):\\d+: (\\w+): (.*)"));
        QRegularExpressionMatch match = regex.match(line);
        QStringList elements = match.capturedTexts();
        elements.erase(elements.begin()); // remove first element
        if (elements.size() != 4) {
            // if parsing fails we clear the list
            return {};
        }

        const auto url = QUrl::fromLocalFile(elements[0]);
        Diagnostic d;
        d.message = elements[3];
        d.severity = DiagnosticSeverity::Warning;
        int ln = elements[1].toInt() - 1;
        d.range = KTextEditor::Range(ln, 0, ln, -1);
        return {.uri = url, .diagnostics = {d}};
    }

    bool isSuccessfulExitCode(int exitCode) const override
    {
        // "0: All files successfully scanned with no issues."
        // "1: All files successfully scanned with some issues."
        return exitCode == 0 || exitCode == 1;
    }

    QString stdinMessages() override
    {
        return {};
    }
};
