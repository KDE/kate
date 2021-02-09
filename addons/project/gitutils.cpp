/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "gitutils.h"

#include <QDateTime>
#include <QDebug>
#include <QProcess>

bool GitUtils::isGitRepo(const QString &repo)
{
    QProcess git;
    git.setWorkingDirectory(repo);
    QStringList args{QStringLiteral("rev-parse"), QStringLiteral("--is-inside-work-tree")};
    git.start(QStringLiteral("git"), args);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        return git.readAll().trimmed() == "true";
    }
    return false;
}

QString GitUtils::getCurrentBranchName(const QString &repo)
{
    QProcess git;
    git.setWorkingDirectory(repo);
    QStringList args{QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"), QStringLiteral("HEAD")};
    git.start(QStringLiteral("git"), args);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        return QString::fromUtf8(git.readAllStandardOutput().trimmed());
    }
    return QString();
}

GitUtils::CheckoutResult GitUtils::checkoutBranch(const QString &repo, const QString &branch)
{
    QProcess git;
    git.setWorkingDirectory(repo);
    QStringList args{QStringLiteral("checkout"), branch};
    git.start(QStringLiteral("git"), args);
    CheckoutResult res;
    res.branch = branch;
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        res.returnCode = git.exitCode();
        res.error = QString::fromUtf8(git.readAllStandardError());
    }
    return res;
}

GitUtils::CheckoutResult GitUtils::checkoutNewBranch(const QString &repo, const QString &newBranch, const QString &fromBranch)
{
    QProcess git;
    git.setWorkingDirectory(repo);
    QStringList args{QStringLiteral("checkout"), QStringLiteral("-q"), QStringLiteral("-b"), newBranch};
    if (!fromBranch.isEmpty()) {
        args.append(fromBranch);
    }
    git.start(QStringLiteral("git"), args);
    CheckoutResult res;
    res.branch = newBranch;
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        res.returnCode = git.exitCode();
        res.error = QString::fromUtf8(git.readAllStandardError());
    }
    return res;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
constexpr auto SkipEmptyParts = QString::SkipEmptyParts;
#else
constexpr auto SkipEmptyParts = Qt::SkipEmptyParts;
#endif

static GitUtils::Branch parseLocalBranch(const QString &raw)
{
    auto data = raw.split(QStringLiteral("{sp}"), SkipEmptyParts);
    return GitUtils::Branch{data.first().remove(QLatin1String("refs/heads/")), QString(), GitUtils::Head};
}

static GitUtils::Branch parseRemoteBranch(const QString &raw)
{
    auto data = raw.split(QStringLiteral("{sp}"), SkipEmptyParts);
    int indexofRemote = data.at(0).indexOf(QLatin1Char('/'), 13);
    return GitUtils::Branch{data.first().remove(QLatin1String("refs/remotes/")), data.at(0).mid(13, indexofRemote - 13), GitUtils::Remote};
}

QVector<GitUtils::Branch> GitUtils::getAllBranchesAndTags(const QString &repo, RefType ref)
{
    // git for-each-ref --format '%(refname) %(objectname) %(*objectname)'
    QProcess git;
    git.setWorkingDirectory(repo);

    QStringList args{QStringLiteral("for-each-ref"), QStringLiteral("--format"), QStringLiteral("%(refname){sp}"), QStringLiteral("--sort=-committerdate")};
    if (ref & RefType::Head) {
        args.append(QStringLiteral("refs/heads"));
    }
    if (ref & RefType::Remote) {
        args.append(QStringLiteral("refs/remotes"));
    }
    if (ref & RefType::Tag) {
        args.append(QStringLiteral("refs/tags"));
        args.append(QStringLiteral("--sort=-taggerdate"));
    }

    git.start(QStringLiteral("git"), args);
    QVector<Branch> branches;
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        QString gitout = QString::fromUtf8(git.readAllStandardOutput());
        QStringList out = gitout.split(QLatin1Char('\n'));

        branches.reserve(out.size());
        // clang-format off
        for (const auto &o : out) {
            if (ref & Head && o.startsWith(QLatin1String("refs/heads"))) {
                branches.append(parseLocalBranch(o));
            } else if (ref & Remote && o.startsWith(QLatin1String("refs/remotes"))) {
                branches.append(parseRemoteBranch(o));
            } else if (ref & Tag && o.startsWith(QLatin1String("refs/tags/"))) {
                int indexofSp = o.indexOf(QLatin1String("{sp}"));
                branches.append({o.mid(10, indexofSp), {}, RefType::Tag});
            }
        }
        // clang-format on
    }

    return branches;
}

QVector<GitUtils::Branch> GitUtils::getAllBranches(const QString &repo)
{
    return getAllBranchesAndTags(repo, static_cast<RefType>(RefType::Head | RefType::Remote));
}
