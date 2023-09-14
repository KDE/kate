// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "hostprocess.h"

#include <QStandardPaths>

#include <KProcess>
#include <KSandbox>
#include <kcoreaddons_version.h>

QString safeExecutableName(const QString &executableName, const QStringList &paths)
{
    return KSandbox::isFlatpak() ? executableName : QStandardPaths::findExecutable(executableName, paths);
}

void startHostProcess(QProcess &proc, QProcess::OpenMode mode)
{
    KSandbox::startHostProcess(proc, mode);
}

void startHostProcess(QProcess &proc, const QString &program, const QStringList &arguments, QProcess::OpenMode mode)
{
    proc.setProgram(program);
    proc.setArguments(arguments);
    startHostProcess(proc, mode);
}
