// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "hostprocess.h"

#include <QStandardPaths>

#include <kcoreaddons_version.h>
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 97, 0)
#include <KSandbox>
#endif
#include <KProcess>

QString safeExecutableName(const QString &executableName, const QStringList &paths)
{
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 97, 0)
    return KSandbox::isFlatpak() ? executableName : QStandardPaths::findExecutable(executableName, paths);
#else
    return QStandardPaths::findExecutable(executableName, paths);
#endif
}

void startHostProcess(QProcess &proc, QProcess::OpenMode mode)
{
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 97, 0)
    KSandbox::startHostProcess(proc, mode);
#else
    KProcess *kprocess = qobject_cast<KProcess *>(&proc);
    if (kprocess) {
        kprocess->setNextOpenMode(mode);
        kprocess->start();
    } else {
        proc.start(mode);
    }
#endif
}

void startHostProcess(QProcess &proc, const QString &program, const QStringList &arguments, QProcess::OpenMode mode)
{
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 97, 0)
    proc.setProgram(program);
    proc.setArguments(arguments);
    startHostProcess(proc, mode);
#else
    proc.start(program, arguments, mode);
#endif
}
