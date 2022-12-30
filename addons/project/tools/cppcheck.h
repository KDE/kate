/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2017 Héctor Mesa Jiménez <hector@lcc.uma.es>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "../kateprojectcodeanalysistool.h"

/**
 * Information provider for cppcheck
 */
class KateProjectCodeAnalysisToolCppcheck : public KateProjectCodeAnalysisTool
{
public:
    explicit KateProjectCodeAnalysisToolCppcheck(QObject *parent = nullptr);

    ~KateProjectCodeAnalysisToolCppcheck() override;

    QString name() const override;

    QString description() const override;

    QString fileExtensions() const override;

    virtual QStringList filter(const QStringList &files) const override;

    QString path() const override;

    QStringList arguments() override;

    QString notInstalledMessage() const override;

    FileDiagnostics parseLine(const QString &line) const override;

    QString stdinMessages() override;
};
