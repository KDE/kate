/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include <kateprojectcodeanalysistool.h>

#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <QRegularExpression>

class HtmlTidyTool : public KateProjectCodeAnalysisTool
{
public:
    using KateProjectCodeAnalysisTool::KateProjectCodeAnalysisTool;

    QString name() const override
    {
        return i18n("Html Tidy");
    }

    QString description() const override
    {
        return i18n("Runs html tidy on current file");
    }

    QString fileExtensions() const override
    {
        return QStringLiteral("html|htm");
    }

    QStringList filter(const QStringList &files) const override
    {
        return files.filter(QRegularExpression(QStringLiteral("\\.(") + fileExtensions() + QStringLiteral(")$")));
    }

    QString path() const override
    {
        return QStringLiteral("tidy");
    }

    QStringList arguments() override
    {
        m_file.clear();
        if (!m_project || !m_mainWindow || !m_mainWindow->activeView()) {
            return {};
        }

        setActualFilesCount(1);

        const QString file = m_mainWindow->activeView()->document()->url().toLocalFile();
        QStringList args{QStringLiteral("-qe"), file};
        m_file = file;
        return args;
    }

    QString notInstalledMessage() const override
    {
        return i18n("Html tidy is not installed. Please use your package manager to install it. See https://www.html-tidy.org/");
    }

    FileDiagnostics parseLine(const QString &line) const override
    {
        static const QRegularExpression re(QStringLiteral("line\\s+(\\d+)\\s+column\\s+(\\d+)\\s+-\\s+(\\w+):(.*)$"));
        const auto result = re.match(line);

        QString lineStr = result.captured(1);
        QString colStr = result.captured(2);
        bool ok;
        int lineNum = lineStr.toInt(&ok) - 1;
        if (!ok) {
            return {};
        }
        int colNum = colStr.toInt(&ok) - 1;
        if (!ok) {
            return {};
        }

        Diagnostic d;
        d.range = KTextEditor::Range(lineNum, colNum, lineNum, colNum + 5);
        d.message = result.captured(4);
        d.severity = result.captured(3) == QLatin1String("Warning") ? DiagnosticSeverity::Warning : DiagnosticSeverity::Error;

        FileDiagnostics fd;
        fd.uri = QUrl::fromLocalFile(m_file);
        fd.diagnostics = {d};
        return fd;
    }

    QString stdinMessages() override
    {
        return {};
    }

    bool isSuccessfulExitCode(int exitCode) const override
    {
        // not documented, so we just return true for all codes
        Q_UNUSED(exitCode)
        return true;
    }

private:
    QString m_file;
};
