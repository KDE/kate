// SPDX-License-Identifier: LGPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "hostprocess.h"

#include <QProcess>
#include <QStandardPaths>

#include "katedebug.h"
#include <KConfigGroup>
#include <KSandbox>
#include <KSharedConfig>

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

void startHostProcess(QProcess &proc, QProcess::OpenMode mode)
{
    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("kate_execution"));
    QStringList command = cg.readEntry("BaseCommand", QStringList());

    // This allows us to run the command under certain tooling like within a docker or flatpak container
    if (!command.isEmpty()) {
        auto args = proc.arguments();
        args.prepend(proc.program());
        proc.setProgram(command.takeFirst());
        proc.setArguments(command + args);
    }

    const QStringList envs = cg.readEntry("Environment", QStringList());
    if (!envs.isEmpty()) {
        auto environment = proc.processEnvironment();
        for (const QString &env : envs) {
            auto splitEnv = env.indexOf(QLatin1Char('='));
            if (splitEnv <= 0) {
                qCWarning(LOG_KATE) << "entry wrongly formatted" << env;
                continue;
            }
            environment.insert(env.left(splitEnv), env.mid(splitEnv + 1));
        }
        proc.setProcessEnvironment(environment);
    }
    KSandbox::startHostProcess(proc, mode);
}

void startHostProcess(QProcess &proc, const QString &program, const QStringList &arguments, QProcess::OpenMode mode)
{
    proc.setProgram(program);
    proc.setArguments(arguments);
    startHostProcess(proc, mode);
}
