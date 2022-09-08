/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include "hostprocess.h"
#include "kateprivate_export.h"

#include <optional>
#include <utility>

class QIcon;

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
KATE_PRIVATE_EXPORT bool setupGitProcess(QProcess &process, const QString &workingDirectory, const QStringList &arguments);

/**
 * helper function to get the git version
 * @param workingDirectory working directory to use for process
 * @return git major and minor version as pair, -1,-1 if infeasible to determine
 */
KATE_PRIVATE_EXPORT std::pair<int, int> getGitVersion(const QString &workingDir);

/**
 * @brief get the git repo base path
 * Returned path has a `/` at the end
 * @param workingDir the dir where
 */
KATE_PRIVATE_EXPORT std::optional<QString> getRepoBasePath(const QString &workingDir);

/**
 * @brief returns the git icon for use in UI
 */
KATE_PRIVATE_EXPORT QIcon gitIcon();
