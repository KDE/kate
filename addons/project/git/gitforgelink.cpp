/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitforgelink.h"

#include "fileutil.h"
#include "gitprocess.h"
#include "hostprocess.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>

namespace {
QString runGit(const QString &repository, const QStringList &arguments)
{
    QProcess process;
    if (!setupGitProcess(process, repository, arguments)) {
        return {};
    }
    startHostProcess(process, QIODevice::ReadOnly);
    if (!process.waitForStarted(3000) || !process.waitForFinished(3000)) {
        process.kill();
        process.waitForFinished();
        return {};
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return {};
    }
    return QString::fromUtf8(process.readAllStandardOutput().trimmed());
}
} // namespace

std::optional<GitForge::Link> GitForge::linkForFile(const QString &filePath, const QList<HostMapping> &hostMappings, std::optional<LineRange> lines)
{
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return std::nullopt;
    }

    const auto root = getRepoBasePath(fileInfo.absolutePath());
    if (!root) {
        return std::nullopt;
    }

    const QString relativePath = QDir(*root).relativeFilePath(fileInfo.absoluteFilePath());
    if (relativePath == QLatin1String("..") || relativePath.startsWith(QLatin1String("../"))) {
        return std::nullopt;
    }
    if (runGit(*root, { QStringLiteral("ls-files"), QStringLiteral("--error-unmatch"), QStringLiteral("--"), relativePath }).isEmpty()) {
        return std::nullopt;
    }

    const QString branch = runGit(*root, { QStringLiteral("symbolic-ref"), QStringLiteral("--quiet"), QStringLiteral("--short"), QStringLiteral("HEAD") });
    QString remote;
    QString ref;
    if (!branch.isEmpty()) {
        remote = runGit(*root, { QStringLiteral("config"), QStringLiteral("branch.%1.remote").arg(branch) });
        const QString merge = runGit(*root, { QStringLiteral("config"), QStringLiteral("branch.%1.merge").arg(branch) });
        if (!remote.isEmpty() && remote != QLatin1String(".") && merge.startsWith(QLatin1String("refs/heads/"))) {
            ref = merge.sliced(11);
        } else {
            remote.clear();
            ref = branch;
        }
    } else {
        ref = runGit(*root, { QStringLiteral("rev-parse"), QStringLiteral("HEAD") });
    }

    if (remote.isEmpty()) {
        const QStringList remotes = runGit(*root, { QStringLiteral("remote") }).split(u'\n', Qt::SkipEmptyParts);
        if (remotes.contains(QStringLiteral("origin"))) {
            remote = QStringLiteral("origin");
        } else if (remotes.size() == 1) {
            remote = remotes.constFirst();
        }
    }
    if (remote.isEmpty() || ref.isEmpty()) {
        return std::nullopt;
    }

    const auto parsedRemote = parseRemote(runGit(*root, { QStringLiteral("remote"), QStringLiteral("get-url"), remote }));
    if (!parsedRemote) {
        return std::nullopt;
    }
    const auto repository = resolveRepository(*parsedRemote, hostMappings);
    if (!repository) {
        return std::nullopt;
    }
    const auto url = blobUrl(*repository, ref, QDir::fromNativeSeparators(relativePath), lines);
    return url ? std::optional<Link>(Link{ *url, repository->provider }) : std::nullopt;
}
