/**
 *  SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "clippy.h"
#include "kateproject.h"

#include <KLocalizedString>

#include <QDir>
#include <QRegularExpression>
#include <QStringLiteral>

Clippy::Clippy(QObject *parent)
    : KateProjectCodeAnalysisTool(parent)
{
}

QString Clippy::name() const
{
    return i18n("Clippy (Rust)");
}

QString Clippy::description() const
{
    return i18n("Clippy is a static analysis tool for Rust code.");
}

QString Clippy::fileExtensions() const
{
    return QStringLiteral("rs");
}

QStringList Clippy::filter(const QStringList &files) const
{
    // js/ts files
    return files.filter(
        QRegularExpression(QStringLiteral("\\.(") + fileExtensions().replace(QStringLiteral("+"), QStringLiteral("\\+")) + QStringLiteral(")$")));
}

QString Clippy::path() const
{
    return QStringLiteral("cargo");
}

QStringList Clippy::arguments()
{
    if (!m_project) {
        return {};
    }

    QStringList args;

    args << QStringLiteral("clippy");
    args << QStringLiteral("--message-format");
    args << QStringLiteral("short");
    args << QStringLiteral("--quiet");
    args << QStringLiteral("--no-deps");
    args << QStringLiteral("--offline");

    setActualFilesCount(m_project->files().size());

    return args;
}

QString Clippy::notInstalledMessage() const
{
    return i18n("Please install 'cargo'.");
}

FileDiagnostics Clippy::parseLine(const QString &line) const
{
    qDebug() << "line" << line;
    // INPUT: src/lib.rs:132:13: warning: field assignment outside of initializer for an instance created with Default::default()
    static QRegularExpression PARSE_LINE_REGEX(QStringLiteral("([^:]+):(\\d+):(\\d+): (\\w+)\\[\\w+\\]: (.*)"));
    // OUT: file, line, column, severity, message
    const QRegularExpressionMatch match = PARSE_LINE_REGEX.match(line);
    QStringList outList = match.capturedTexts();
    if (outList.size() != 6) {
        return {};
    }
    outList.erase(outList.begin()); // remove first element

    QUrl uri = QUrl::fromLocalFile(m_project->baseDir() + QStringLiteral("/") + outList[0]);
    Diagnostic d;
    int ln = outList[1].toInt() - 1;
    int col = outList[2].toInt() - 1;
    d.range = {ln, col, ln, col};
    d.severity = DiagnosticSeverity::Warning;
    d.message = outList[4];
    d.source = QStringLiteral("clippy");
    return {uri, {d}};
}

QString Clippy::stdinMessages()
{
    return QString();
}
