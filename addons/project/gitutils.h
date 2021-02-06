#ifndef GITUTILS_H
#define GITUTILS_H

#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QString>

namespace GitUtils
{
static bool isGitRepo(const QString &repo)
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

static QString getCurrentBranchName(const QString &repo)
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

static int checkoutBranch(const QString &repo, const QString &branch)
{
    QProcess git;
    git.setWorkingDirectory(repo);
    QStringList args{QStringLiteral("checkout"), branch};
    git.start(QStringLiteral("git"), args);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        return git.exitCode();
    }
    return -1;
}

static QStringList getAllBranches(const QString &repo)
{
    QProcess git;
    QStringList args{QStringLiteral("branch"), QStringLiteral("--all")};
    git.setWorkingDirectory(repo);

    git.start(QStringLiteral("git"), args);
    QStringList branches;
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        QList<QByteArray> out = git.readAllStandardOutput().split('\n');
        for (const QByteArray &br : out) {
            auto branch = br.trimmed();
            if (!branch.isEmpty() && !branch.startsWith("* ")) {
                branches.append(QString::fromUtf8(branch.trimmed()));
            }
        }
    }
    return branches;
}

}

#endif // GITUTILS_H
