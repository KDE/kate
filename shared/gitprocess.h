/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QProcess>
#include <QStandardPaths>

/**
 * small helper function to setup a QProcess based "git" command.
 * you pass working directory & arguments
 * it will additionally setup stuff like GIT_OPTIONAL_LOCKS=0
 * after this, you can just call "start" or "startDetached" for the process.
 *
 * @param process process to setup for git
 * @param workingDirectory working directory to use for process
 * @param arguments arguments to pass to git
 * @return could set setup the process or did that fail, e.g. because the git executable is not available?
 */
inline bool setupGitProcess(QProcess &process, const QString &workingDirectory, const QStringList &arguments)
{
    // only use git from PATH
    static const auto gitExecutable = QStandardPaths::findExecutable(QStringLiteral("git"));
    if (gitExecutable.isEmpty()) {
        // ensure we have no valid QProcess setup
        process.setProgram(QString());
        return false;
    }

    // setup program and arguments, ensure we do run git in the right working directory
    process.setProgram(gitExecutable);
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
    return true;
}
