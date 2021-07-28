/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QProcess>

/**
 * small helper function to setup a QProcess based "git" command.
 * you pass working directory & arguments
 * it will additionally setup stuff like GIT_OPTIONAL_LOCKS=0
 * after this, you can just call "start" or "startDetached" for the process.
 *
 * @param process process to setup for git
 * @param workingDirectory working directory to use for process
 * @param arguments arguments to pass to git
 */
inline void setupGitProcess(QProcess &process, const QString &workingDirectory, const QStringList &arguments)
{
    process.setProgram(QStringLiteral("git"));
    process.setWorkingDirectory(workingDirectory);
    process.setArguments(arguments);

    /**
     * from the git manual:
     *
     * If set to 0, Git will complete any requested operation without performing any optional sub-operations that require taking a lock.
     * For example, this will prevent git status from refreshing the index as a side effect.
     * This is useful for processes running in the background which do not want to cause lock contention with other operations on the repository.
     * Defaults to 1.
     *
     * we use the env var as this is compatible even for "ancient" git versions pre 2.15.2
     */
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("GIT_OPTIONAL_LOCKS"), QStringLiteral("0"));
    process.setProcessEnvironment(env);
}
