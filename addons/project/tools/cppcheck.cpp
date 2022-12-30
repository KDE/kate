/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2017 Héctor Mesa Jiménez <hector@lcc.uma.es>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "cppcheck.h"
#include "kateproject.h"

#include <KLocalizedString>
#include <QRegularExpression>
#include <QThread>

KateProjectCodeAnalysisToolCppcheck::KateProjectCodeAnalysisToolCppcheck(QObject *parent)
    : KateProjectCodeAnalysisTool(parent)
{
}

KateProjectCodeAnalysisToolCppcheck::~KateProjectCodeAnalysisToolCppcheck()
{
}

QString KateProjectCodeAnalysisToolCppcheck::name() const
{
    return i18n("Cppcheck (C++)");
}

QString KateProjectCodeAnalysisToolCppcheck::description() const
{
    return i18n("Cppcheck is a static analysis tool for C/C++ code");
}

QString KateProjectCodeAnalysisToolCppcheck::fileExtensions() const
{
    return QStringLiteral("cpp|cxx|cc|c++|c|tpp|txx");
}

QStringList KateProjectCodeAnalysisToolCppcheck::filter(const QStringList &files) const
{
    // c++ files
    return files.filter(
        QRegularExpression(QStringLiteral("\\.(") + fileExtensions().replace(QStringLiteral("+"), QStringLiteral("\\+")) + QStringLiteral(")$")));
}

QString KateProjectCodeAnalysisToolCppcheck::path() const
{
    return QStringLiteral("cppcheck");
}

QStringList KateProjectCodeAnalysisToolCppcheck::arguments()
{
    QStringList _args;

    _args << QStringLiteral("-q") << QStringLiteral("-f") << QStringLiteral("-j") + QString::number(QThread::idealThreadCount())
          << QStringLiteral("--inline-suppr") << QStringLiteral("--enable=all")
          << QStringLiteral("--template={file}////{line}////{column}////{severity}////{id}////{message}") << QStringLiteral("--file-list=-");

    return _args;
}

QString KateProjectCodeAnalysisToolCppcheck::notInstalledMessage() const
{
    return i18n("Please install 'cppcheck'.");
}

FileDiagnostics KateProjectCodeAnalysisToolCppcheck::parseLine(const QString &line) const
{
    const QStringList elements = line.split(QLatin1String("////"), Qt::SkipEmptyParts);
    if (elements.size() < 4) {
        return {};
    }

    Diagnostic d;
    const auto url = QUrl::fromLocalFile(elements[0]);
    int ln = elements[1].toInt() - 1;
    int col = elements[2].toInt() - 1;
    d.range = KTextEditor::Range(ln, col, ln, col);
    d.source = QStringLiteral("cppcheck");
    d.code = elements[4];
    d.message = elements[5];
    if (elements[3].startsWith(QLatin1String("warn"))) {
        d.severity = DiagnosticSeverity::Warning;
    } else if (elements[3].startsWith(QLatin1String("error"))) {
        d.severity = DiagnosticSeverity::Error;
    } else {
        d.severity = DiagnosticSeverity::Information;
    }
    return {url, {d}};
}

QString KateProjectCodeAnalysisToolCppcheck::stdinMessages()
{
    // filenames are written to stdin (--file-list=-)

    if (!m_project) {
        return QString();
    }

    auto &&fileList = filter(m_project->files());
    setActualFilesCount(fileList.size());
    return fileList.join(QLatin1Char('\n'));
}
