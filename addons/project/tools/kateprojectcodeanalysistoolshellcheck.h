/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2018 Gregor Mi <codestruct@posteo.org>
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

#pragma once

#include "../kateprojectcodeanalysistool.h"

/**
 * Information provider for shellcheck
 */
class KateProjectCodeAnalysisToolShellcheck : public KateProjectCodeAnalysisTool
{
public:
    explicit KateProjectCodeAnalysisToolShellcheck(QObject *parent = nullptr);

    ~KateProjectCodeAnalysisToolShellcheck() override;

    QString name() const override;

    QString description() const override;

    QString fileExtensions() const override;

    virtual QStringList filter(const QStringList &files) const override;

    QString path() const override;

    QStringList arguments() override;

    QString notInstalledMessage() const override;

    QStringList parseLine(const QString &line) const override;

    bool isSuccessfulExitCode(int exitCode) const override;

    QString stdinMessages() override;
};
