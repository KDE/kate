/**
 *  SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <kateprojectcodeanalysistool.h>

class ESLint : public KateProjectCodeAnalysisTool
{
public:
    explicit ESLint(QObject *parent = nullptr);

    QString name() const override;

    QString description() const override;

    QString fileExtensions() const override;

    QStringList filter(const QStringList &files) const override;

    QString path() const override;

    QStringList arguments() override;

    QString notInstalledMessage() const override;

    FileDiagnostics parseLine(const QString &line) const override;

    bool isSuccessfulExitCode(int c) const override
    {
        return c == 1 || c == 0;
    }

    QString stdinMessages() override;
};
