/**
 *  SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <kateprojectcodeanalysistool.h>

class Clippy : public KateProjectCodeAnalysisTool
{
public:
    explicit Clippy(QObject *parent = nullptr);

    ~Clippy() override = default;

    QString name() const override;

    QString description() const override;

    QString fileExtensions() const override;

    QStringList filter(const QStringList &files) const override;

    QString path() const override;

    QStringList arguments() override;

    QString notInstalledMessage() const override;

    FileDiagnostics parseLine(const QString &line) const override;

    QString stdinMessages() override;

    bool isSuccessfulExitCode(int exitCode) const override
    {
        return 101 == exitCode;
    }
};
