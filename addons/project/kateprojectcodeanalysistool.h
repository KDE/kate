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

#ifndef KATE_PROJECT_CODE_ANALYSIS_TOOL_H
#define KATE_PROJECT_CODE_ANALYSIS_TOOL_H

#include <QString>
#include <QStringList>

#include "kateproject.h"

/**
 * Information provider for a code analysis tool
 */
class KateProjectCodeAnalysisTool: public QObject
{
    Q_OBJECT
protected:
    explicit KateProjectCodeAnalysisTool(QObject *parent = nullptr);

    /**
     * Current project
     */
    KateProject *m_project;

public:
    virtual ~KateProjectCodeAnalysisTool();

    /**
     * bind to this project
     * @param project project this tool will analyse
     */
    virtual void setProject(KateProject *project);

    /**
     * @return tool descriptive name
     */
    virtual QString name() = 0;

    /**
     * filter relevant files
     * @param files set of files in project
     * @return relevant files that can be analysed
     */
    virtual QStringList filter(const QStringList &files) = 0;

    /**
     * @return tool path
     */
    virtual QString path() = 0;

    /**
     * @return arguments required for the tool
     */
    virtual QStringList arguments() = 0;

    /**
     * @return warning message when the tool is not installed
     */
    virtual QString notInstalledMessage() = 0;

    /**
     * parse output line
     * @param line
     * @return file, line, severity, message
     */
    virtual QStringList parseLine(const QString &line) = 0;

    /**
     * @return messages passed to the tool through stdin
     */
    virtual QString stdinMessages() = 0;
};

Q_DECLARE_METATYPE(KateProjectCodeAnalysisTool*)

#endif // KATE_PROJECT_CODE_ANALYSIS_TOOL_H
