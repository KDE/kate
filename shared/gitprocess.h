/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QProcess>
#include <QStandardPaths>
#include <QRegularExpression>

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
bool setupGitProcess(QProcess &process, const QString &workingDirectory, const QStringList &arguments);

/**
 * helper function to get the git version
 * @param workingDirectory working directory to use for process
 * @return git major and minor version as pair, -1,-1 if infeasible to determine
 */
std::pair<int, int> getGitVersion(const QString &workingDir);
