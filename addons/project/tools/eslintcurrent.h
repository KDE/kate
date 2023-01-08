/**
 *  SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef KATEPROJECTCODEANALYSISTOOLESLINT_H
#define KATEPROJECTCODEANALYSISTOOLESLINT_H

#include <kateprojectcodeanalysistool.h>

class ESLintCurrentFile : public KateProjectCodeAnalysisTool
{
public:
    explicit ESLintCurrentFile(QObject *parent = nullptr);

    ~ESLintCurrentFile() override = default;

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
        return c == 1;
    }

    QString stdinMessages() override;
};

#endif // KATEPROJECTCODEANALYSISTOOLESLINT_H
