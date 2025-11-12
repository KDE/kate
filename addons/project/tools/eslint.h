/**
 *  SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#pragma once

#include "kateproject.h"
#include <kateprojectcodeanalysistool.h>

#include <KLocalizedString>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

class ESLint : public KateProjectCodeAnalysisTool
{
public:
    using KateProjectCodeAnalysisTool::KateProjectCodeAnalysisTool;

    QString name() const override
    {
        return i18n("ESLint");
    }

    QString description() const override
    {
        return i18n("ESLint is a static analysis tool & style guide enforcer for JavaScript/Typescript code.");
    }

    QString fileExtensions() const override
    {
        return QStringLiteral("js|jsx|ts|tsx");
    }

    QStringList filter(const QStringList &files) const override
    {
        // js/ts files
        return files.filter(
            QRegularExpression(QStringLiteral("\\.(") + fileExtensions().replace(QStringLiteral("+"), QStringLiteral("\\+")) + QStringLiteral(")$")));
    }

    QString path() const override
    {
        return QStringLiteral("npx");
    }

    QStringList arguments() override
    {
        if (!m_project) {
            return {};
        }

        QStringList args{
            QStringLiteral("eslint"),
            QStringLiteral("-f"),
            QStringLiteral("json"),
        };

        const QStringList fileList = filter(m_project->files());
        setActualFilesCount(fileList.size());
        return args << fileList;
    }

    QString notInstalledMessage() const override
    {
        return i18n("Please install 'eslint'.");
    }

    FileDiagnostics parseLine(const QString &line) const override
    {
        QJsonParseError e;
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &e);
        if (e.error != QJsonParseError::NoError) {
            return {};
        }

        const auto arr = doc.array();
        if (arr.empty()) {
            return {};
        }
        auto obj = arr.at(0).toObject();
        QUrl uri = QUrl::fromLocalFile(obj.value(QStringLiteral("filePath")).toString());
        if (!uri.isValid()) {
            return {};
        }
        const auto messages = obj.value(QStringLiteral("messages")).toArray();
        if (messages.empty()) {
            return {};
        }

        QList<Diagnostic> diags;
        diags.reserve(messages.size());
        for (const auto &m : messages) {
            const auto msg = m.toObject();
            if (msg.isEmpty()) {
                continue;
            }
            const int startLine = msg.value(QStringLiteral("line")).toInt() - 1;
            const int startColumn = msg.value(QStringLiteral("column")).toInt() - 1;
            const int endLine = msg.value(QStringLiteral("endLine")).toInt() - 1;
            const int endColumn = msg.value(QStringLiteral("endColumn")).toInt() - 1;
            Diagnostic diag;
            diag.range = {startLine, startColumn, endLine, endColumn};
            if (!diag.range.isValid()) {
                continue;
            }
            diag.code = msg.value(QStringLiteral("ruleId")).toString();
            diag.message = msg.value(QStringLiteral("message")).toString();
            diag.source = QStringLiteral("eslint");
            const int severity = msg.value(QStringLiteral("severity")).toInt();
            if (severity == 1) {
                diag.severity = DiagnosticSeverity::Warning;
            } else if (severity == 2) {
                diag.severity = DiagnosticSeverity::Error;
            } else {
                // fallback, even though there is no other severity
                diag.severity = DiagnosticSeverity::Information;
            }
            diags << diag;
        }
        return {.uri = uri, .diagnostics = diags};
    }

    QString stdinMessages() override
    {
        return {};
    }
};
