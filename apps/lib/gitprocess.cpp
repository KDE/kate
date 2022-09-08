/*
    SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "gitprocess.h"

#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

#include <QIcon>

#include <KIconLoader>

bool setupGitProcess(QProcess &process, const QString &workingDirectory, const QStringList &arguments)
{
    // only use git from PATH
    static const auto gitExecutable = safeExecutableName(QStringLiteral("git"));
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

// internal helper for the external caching accessor
static std::pair<int, int> getGitVersionUncached(const QString &workingDir)
{
    QProcess git;
    if (!setupGitProcess(git, workingDir, {QStringLiteral("--version")})) {
        return {-1, -1};
    }

    // try to run, no version output feasible if not possible
    startHostProcess(git, QProcess::ReadOnly);
    if (!git.waitForStarted() || !git.waitForFinished() || git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
        return {-1, -1};
    }

    // match the version output
    const QString gitVersion = QString::fromUtf8(git.readAllStandardOutput());
    const QRegularExpression gitRegex(QStringLiteral("git version\\s*(\\d+).(\\d+).(\\d+)+.*"));
    const QRegularExpressionMatch gitMatch = gitRegex.match(gitVersion);

    bool okMajor = false;
    bool okMinor = false;
    const int versionMajor = gitMatch.captured(1).toInt(&okMajor);
    const int versionMinor = gitMatch.captured(2).toInt(&okMinor);
    if (okMajor && okMinor) {
        return {versionMajor, versionMinor};
    }

    // no version properly detected
    return {-1, -1};
}

std::pair<int, int> getGitVersion(const QString &workingDir)
{
    // cache internal result to avoid expensive recalculation
    static const auto cachedVersion = getGitVersionUncached(workingDir);
    return cachedVersion;
}

std::optional<QString> getRepoBasePath(const QString &repo)
{
    /* This call is intentionally blocking because we need git path for everything else */
    QProcess git;
    if (!setupGitProcess(git, repo, {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")})) {
        return std::nullopt;
    }

    startHostProcess(git, QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            return std::nullopt;
        }
        QString dotGitPath = QString::fromUtf8(git.readAllStandardOutput().trimmed());
        if (!dotGitPath.endsWith(QLatin1Char('/'))) {
            dotGitPath.append(QLatin1Char('/'));
        }
        return dotGitPath;
    }
    return std::nullopt;
}

QIcon gitIcon()
{
    static const auto icon = KDE::icon(QStringLiteral(":/icons/icons/sc-apps-git.svg"));
    return icon;
}
