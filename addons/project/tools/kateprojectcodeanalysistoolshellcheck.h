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

    virtual ~KateProjectCodeAnalysisToolShellcheck() override;

    virtual QString name() override;

    virtual QString description() const override;

    virtual QString fileExtensions() override;

    virtual QStringList filter(const QStringList &files) override;

    virtual QString path() override;

    virtual QStringList arguments() override;

    virtual QString notInstalledMessage() override;

    virtual QStringList parseLine(const QString &line) override;

    virtual bool isSuccessfulExitCode(int exitCode) override;

    virtual QString stdinMessages() override;
};
