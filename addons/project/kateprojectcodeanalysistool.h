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
     * @param project project this tool will analyze
     */
    virtual void setProject(KateProject *project);

    /**
     * @return tool descriptive name
     */
    virtual QString name() const = 0;

    /**
     * @return tool short description
     */
    virtual QString description() const = 0;

    /**
     * @returns a string containing the file extensions this
     * tool should be run, separated by '|',
     * e.g. "cpp|cxx"
     * NOTE that this is used directly as part of a regular expression.
     * If more flexibility is required this method probably will change
     */
    virtual QString fileExtensions() const = 0;

    /**
     * filter relevant files
     * @param files set of files in project
     * @return relevant files that can be analyzed
     */
    virtual QStringList filter(const QStringList &files) const = 0;

    /**
     * @return tool path
     */
    virtual QString path() const = 0;

    /**
     * @return arguments required for the tool
     * NOTE that this method is not const because here setActualFilesCount might be called
     */
    virtual QStringList arguments() = 0;

    /**
     * @return warning message when the tool is not installed
     */
    virtual QString notInstalledMessage() const = 0;

    /**
     * parse output line
     * @param line
     * @return file, line, severity, message
     */
    virtual QStringList parseLine(const QString &line) const = 0;

    /**
     * Tells the tool runner if the returned process exit code
     * was a successful one.
     *
     * The default implementation returns true on exitCode 0.
     *
     * Override this method for a tool that use a non-zero exit code
     * e.g. if the processing itself was successful but not all files
     * had no linter errors.
     */
    virtual bool isSuccessfulExitCode(int exitCode) const;

    /**
     * @return messages passed to the tool through stdin
     * This is used when the files are not passed as arguments to the tool.
     *
     * NOTE that this method is not const because here setActualFilesCount might be called
     */
    virtual QString stdinMessages() = 0;

    /**
     * @returns the number of files to be processed after the filter
     * has been applied
     */
    int getActualFilesCount() const;

    /**
     * To be called by derived classes
     */
    void setActualFilesCount(int count);

private:
    int m_filesCount = 0;
};

Q_DECLARE_METATYPE(KateProjectCodeAnalysisTool*)

#endif // KATE_PROJECT_CODE_ANALYSIS_TOOL_H
