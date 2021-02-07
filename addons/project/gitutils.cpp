#include "gitutils.h"

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

int GitUtils::checkoutBranch(const QString &repo, const QString &branch)
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

QVector<GitUtils::Branch> GitUtils::getAllBranchesAndTags(const QString &repo, RefType ref)
{
    // git for-each-ref --format '%(refname) %(objectname) %(*objectname)'
    QProcess git;
    git.setWorkingDirectory(repo);
    QStringList args{QStringLiteral("for-each-ref"), QStringLiteral("--format"), QStringLiteral("%(refname)"), QStringLiteral("--sort=-committerdate")};

    git.start(QStringLiteral("git"), args);
    QVector<Branch> branches;
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        QString gitout = QString::fromUtf8(git.readAllStandardOutput());
        QVector<QStringRef> out = gitout.splitRef(QLatin1Char('\n'));
        static const QRegularExpression headRe(QStringLiteral("^refs/heads/([^ ]+)$"));
        static const QRegularExpression remoteRe(QStringLiteral("^refs/remotes/([^/]+)/([^ ]+)"));
        static const QRegularExpression tagRe(QStringLiteral("^refs/tags/([^ ]+)$"));

        branches.reserve(out.size());
        QRegularExpressionMatch m;
        // clang-format off
        for (const auto &o : out) {
            if (ref & Head && (m = headRe.match(o)).hasMatch()) {
                branches.append({m.captured(1),
                                 QString(), // no remote
                                 RefType::Head,
                                -1});
            } else if (ref & Remote && (m = remoteRe.match(o)).hasMatch()) {
                branches.append({m.captured(1).append(QLatin1Char('/') + m.captured(2)),
                                 m.captured(1),
                                 RefType::Remote,
                                -1});
            } else if (ref & Tag && (m = tagRe.match(o)).hasMatch()) {
                branches.append({m.captured(1),
                                 QString(), // no remote
                                 RefType::Tag,
                                -1});
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
