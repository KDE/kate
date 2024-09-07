/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include "kateproject.h"
#include <kateprojectcodeanalysistool.h>

#include <KLocalizedString>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

class RuffTool : public KateProjectCodeAnalysisTool
{
public:
    using KateProjectCodeAnalysisTool::KateProjectCodeAnalysisTool;

    QString name() const override
    {
        return i18n("Ruff (Python)");
    }

    QString description() const override
    {
        return i18n("An extremely fast Python linter");
    }

    QString fileExtensions() const override
    {
        return QStringLiteral("py");
    }

    QStringList filter(const QStringList &files) const override
    {
        return files.filter(QRegularExpression(QStringLiteral("\\.(") + fileExtensions() + QStringLiteral(")$")));
    }

    QString path() const override
    {
        return QStringLiteral("ruff");
    }

    QStringList arguments() override
    {
        if (!m_project) {
            return {};
        }
        const QStringList fileList = filter(m_project->files());
        setActualFilesCount(fileList.size());

        return {QStringLiteral("check"), QStringLiteral("--output-format"), QStringLiteral("json-lines"), m_project->baseDir()};
    }

    QString notInstalledMessage() const override
    {
        return i18n("Ruff is not installed. Please use your package manager to install it. See https://docs.astral.sh/ruff/installation/");
    }

    FileDiagnostics parseLine(const QString &line) const override
    {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            return {};
        }
        QJsonObject json = doc.object();
        const QString code = json.value(QLatin1String("code")).toString();
        const QUrl filename = QUrl::fromLocalFile(json.value(QLatin1String("filename")).toString());
        const QString message = json.value(QLatin1String("message")).toString();
        const KTextEditor::Range range = [&] {
            KTextEditor::Range ret;
            const auto location = json.value(QLatin1String("location")).toObject();
            ret.setStart({location.value(QLatin1String("row")).toInt() - 1, location.value(QLatin1String("column")).toInt() - 1});

            const auto end_location = json.value(QLatin1String("end_location")).toObject();
            ret.setEnd({end_location.value(QLatin1String("row")).toInt() - 1, end_location.value(QLatin1String("column")).toInt() - 1});
            return ret;
        }();
        // from ruff_server/src/lint.rs severity()
        DiagnosticSeverity severity = DiagnosticSeverity::Warning;
        if (code == QLatin1String("F821") || code == QLatin1String("E902") || code == QLatin1String("E999")) {
            severity = DiagnosticSeverity::Error;
        }

        Diagnostic d;
        d.code = code;
        d.message = message;
        d.range = range;
        d.severity = severity;
        return {filename, {d}};
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
};
