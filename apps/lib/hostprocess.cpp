// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "hostprocess.h"
#include <KConfigGroup>
#include <KSharedConfig>

#include <QStandardPaths>

#include <KSandbox>

QString safeExecutableName(const QString &executableName, const QStringList &paths)
{
    if (KSandbox::isFlatpak()) {
        QProcess process;
        startHostProcess(process, QStringLiteral("which"), {executableName});
        process.waitForFinished();
        return process.exitCode() == 0 ? QString::fromUtf8(process.readAllStandardOutput().trimmed()) : QString();
    }
    return QStandardPaths::findExecutable(executableName, paths);
}

QString safePrefixedExecutableName(const QString &executableName)
{
    KConfigGroup cgGeneral = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("General"));
    const auto containerPrefix = cgGeneral.readEntry("Selected Container", QString());
    auto containerItems = containerPrefix.split(QStringLiteral(" "));
    auto prog = (containerItems.takeFirst());
    auto args = containerItems;
    args.append(QStringLiteral("which"));
    args.append(executableName);

    QProcess process;
    qWarning() << "safePrefixedExecutableName" << prog << args;
    startHostProcess(process, prog, args);
    process.waitForFinished();
    return process.exitCode() == 0 ? QString::fromUtf8(process.readAllStandardOutput().trimmed()) : QString();
}

void startHostProcess(QProcess &proc, QProcess::OpenMode mode)
{
    KSandbox::startHostProcess(proc, mode);
}

void startHostProcess(QProcess &proc, const QString &program, const QStringList &arguments, QProcess::OpenMode mode)
{
    KConfigGroup cgGeneral = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("General"));
    proc.setProgram(program);
    proc.setArguments(arguments);
    qWarning() << "Start host process" << proc.program() << proc.arguments();
    startHostProcess(proc, mode);
}

void startPrefixedHostProcess(QProcess &proc, const QString &program, const QStringList &arguments, QProcess::OpenMode mode)
{
    KConfigGroup cgGeneral = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("General"));
    const auto containerPrefix = cgGeneral.readEntry("Selected Container", QString());
    auto containerItems = containerPrefix.split(QStringLiteral(" "));
    auto prog = (containerItems.takeFirst());
    auto args = containerItems;
    args.append(program);
    args.append(arguments);

    proc.setProgram(prog);
    proc.setArguments(args);
    qWarning() << "Start prefixed host process" << proc.program() << proc.arguments();
    startHostProcess(proc, mode);
}
