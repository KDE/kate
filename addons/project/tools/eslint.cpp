/**
 *  SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "eslint.h"
#include "kateproject.h"

#include <KLocalizedString>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

ESLint::ESLint(QObject *parent)
    : KateProjectCodeAnalysisTool(parent)
{
}

QString ESLint::name() const
{
    return i18n("ESLint");
}

QString ESLint::description() const
{
    return i18n("ESLint is a static analysis tool & style guide enforcer for JavaScript/Typescript code.");
}

QString ESLint::fileExtensions() const
{
    return QStringLiteral("js|jsx|ts|tsx");
}

QStringList ESLint::filter(const QStringList &files) const
{
    // js/ts files
    return files.filter(
        QRegularExpression(QStringLiteral("\\.(") + fileExtensions().replace(QStringLiteral("+"), QStringLiteral("\\+")) + QStringLiteral(")$")));
}

QString ESLint::path() const
{
    return QStringLiteral("npx");
}

QStringList ESLint::arguments()
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

QString ESLint::notInstalledMessage() const
{
    return i18n("Please install 'eslint'.");
}

FileDiagnostics ESLint::parseLine(const QString &line) const
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
    return {uri, diags};
}

QString ESLint::stdinMessages()
{
    return QString();
}
