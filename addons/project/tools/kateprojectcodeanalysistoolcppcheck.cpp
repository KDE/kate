/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2017 Héctor Mesa Jiménez <hector@lcc.uma.es>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kateprojectcodeanalysistoolcppcheck.h"

#include <QRegularExpression>
#include <klocalizedstring.h>

KateProjectCodeAnalysisToolCppcheck::KateProjectCodeAnalysisToolCppcheck(QObject *parent)
    : KateProjectCodeAnalysisTool(parent)
{

}

KateProjectCodeAnalysisToolCppcheck::~KateProjectCodeAnalysisToolCppcheck()
{

}

QString KateProjectCodeAnalysisToolCppcheck::name()
{
    return i18n("cppcheck");
}

QStringList KateProjectCodeAnalysisToolCppcheck::filter(const QStringList &files)
{
    // c++ files
    return files.filter(QRegularExpression(QStringLiteral("\\.(cpp|cxx|cc|c\\+\\+|c|tpp|txx)$")));
}

QString KateProjectCodeAnalysisToolCppcheck::path()
{
    return QStringLiteral("cppcheck");
}

QStringList KateProjectCodeAnalysisToolCppcheck::arguments()
{
    QStringList _args;

    _args << QStringLiteral("-q")
          << QStringLiteral("--inline-suppr")
          << QStringLiteral("--enable=all")
          << QStringLiteral("--template={file}////{line}////{severity}////{message}")
          << QStringLiteral("--file-list=-");

    return _args;
}

QString KateProjectCodeAnalysisToolCppcheck::notInstalledMessage()
{
    return i18n("Please install 'cppcheck'.");
}

QStringList KateProjectCodeAnalysisToolCppcheck::parseLine(const QString &line)
{
    return line.split(QRegExp(QStringLiteral("////")), QString::SkipEmptyParts);
}

QString KateProjectCodeAnalysisToolCppcheck::stdinMessages()
{
    // filenames are written to stdin (--file-list=-)

    if (!m_project) {
        return QString();
    }

    return filter(m_project->files()).join(QStringLiteral("\n"));
}
