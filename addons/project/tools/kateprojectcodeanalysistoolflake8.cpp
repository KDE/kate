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

#include "kateprojectcodeanalysistoolflake8.h"

#include <QRegularExpression>
#include <klocalizedstring.h>

KateProjectCodeAnalysisToolFlake8::KateProjectCodeAnalysisToolFlake8(QObject *parent)
    : KateProjectCodeAnalysisTool(parent)
{

}

KateProjectCodeAnalysisToolFlake8::~KateProjectCodeAnalysisToolFlake8()
{

}

QString KateProjectCodeAnalysisToolFlake8::name()
{
    return i18n("flake8");
}

QStringList KateProjectCodeAnalysisToolFlake8::filter(const QStringList &files)
{
    // for now we expect files with extension
    return files.filter(QRegularExpression(QStringLiteral("\\.py$")));
}

QString KateProjectCodeAnalysisToolFlake8::path()
{
    /*
     * for now, only the executable in the path can be called,
     * but it would be great to be able to specify a version
     * installed in a virtual environment
     */
    return QStringLiteral("flake8");
}

QStringList KateProjectCodeAnalysisToolFlake8::arguments()
{
    QStringList _args;

    _args << QStringLiteral("--exit-zero")
          /*
           * translating a flake8 code to a severity level is subjective,
           * so the code is provided as a severity level.
           */
          << QStringLiteral("--format=%(path)s////%(row)d////%(code)s////%(text)s");

    if (m_project) {
        _args.append(filter(m_project->files()));
    }

    return _args;
}

QString KateProjectCodeAnalysisToolFlake8::notInstalledMessage()
{
    return i18n("Please install 'flake8'.");
}

QStringList KateProjectCodeAnalysisToolFlake8::parseLine(const QString &line)
{
    return line.split(QRegExp(QStringLiteral("////")), QString::SkipEmptyParts);
}

QString KateProjectCodeAnalysisToolFlake8::stdinMessages()
{
    return QString();
}
