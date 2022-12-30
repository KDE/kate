/**
 *  SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "eslintcurrent.h"
#include "kateproject.h"

#include <KLocalizedString>

#include <KTextEditor/MainWindow>
#include <QDir>
#include <QRegularExpression>

ESLintCurrentFile::ESLintCurrentFile(QObject *parent)
    : KateProjectCodeAnalysisTool(parent)
{
}

QString ESLintCurrentFile::name() const
{
    return i18n("ESLint current file");
}

QString ESLintCurrentFile::description() const
{
    return i18n("ESLint is a static analysis tool & style guide enforcer for JavaScript/Typescript code.");
}

QString ESLintCurrentFile::fileExtensions() const
{
    return QStringLiteral("js|jsx|ts|tsx");
}

QStringList ESLintCurrentFile::filter(const QStringList &files) const
{
    // js/ts files
    return files.filter(
        QRegularExpression(QStringLiteral("\\.(") + fileExtensions().replace(QStringLiteral("+"), QStringLiteral("\\+")) + QStringLiteral(")$")));
}

QString ESLintCurrentFile::path() const
{
    return QStringLiteral("npx");
}

QStringList ESLintCurrentFile::arguments()
{
    if (!m_project || !m_mainWindow->activeView()) {
        return {};
    }

    QStringList args;

    args << QStringLiteral("eslint");
    args << QStringLiteral("-f");
    args << QStringLiteral("compact");
    args << QStringLiteral("--cache");
    setActualFilesCount(1);
    const QString file = m_mainWindow->activeView()->document()->url().toLocalFile();
    if (file.isEmpty()) {
        return {};
    }
    args.append(file);
    return args;
}

QString ESLintCurrentFile::notInstalledMessage() const
{
    return i18n("Please install 'eslint'.");
}

FileDiagnostics ESLintCurrentFile::parseLine(const QString &line) const
{
    // INPUT: /path/to/file.js: line 7, col 2, Error - Unnecessary semicolon. (no-extra-semi)
    static const QRegularExpression PARSE_LINE_REGEX(QStringLiteral("([^:]+): line (\\d+), col (\\d+), (\\w+) - (.+)"));
    // OUT: file, line, column, severity, message
    QRegularExpressionMatch match = PARSE_LINE_REGEX.match(line);
    QStringList outList = match.capturedTexts();
    if (outList.size() != 6) {
        return {};
    }
    outList.erase(outList.begin()); // remove first element

    QUrl uri = QUrl::fromLocalFile(outList[0]);
    Diagnostic d;
    int ln = outList[1].toInt() - 1;
    int col = outList[2].toInt() - 1;
    d.range = {ln, col, ln, col};
    if (outList[3].startsWith(QLatin1String("Error"))) {
        d.severity = DiagnosticSeverity::Error;
    } else {
        d.severity = DiagnosticSeverity::Warning;
    }
    d.message = outList[4];
    d.source = QStringLiteral("eslint");

    return {uri, {d}};
}

QString ESLintCurrentFile::stdinMessages()
{
    return QString();
}
