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

QVector<GitUtils::Branch> GitUtils::getAllBranches(const QString &repo, RefType ref)
{
    // git for-each-ref --format '%(refname) %(objectname) %(*objectname)'
    QProcess git;
    git.setWorkingDirectory(repo);
    QStringList args{QStringLiteral("for-each-ref"), QStringLiteral("--format"), QStringLiteral("%(refname) %(objectname) %(*objectname)")};

    git.start(QStringLiteral("git"), args);
    QVector<Branch> branches;
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        QString gitout = QString::fromUtf8(git.readAllStandardOutput());
        QVector<QStringRef> out = gitout.splitRef(QLatin1Char('\n'));
        static const QRegularExpression headRe(QStringLiteral("^refs/heads/([^ ]+) ([0-9a-f]{40}) ([0-9a-f]{40})?$"));
        static const QRegularExpression remoteRe(QStringLiteral("^refs/remotes/([^/]+)/([^ ]+) ([0-9a-f]{40}) ([0-9a-f]{40})?$"));
        static const QRegularExpression tagRe(QStringLiteral("^refs/tags/([^ ]+) ([0-9a-f]{40}) ([0-9a-f]{40})?$"));

        branches.reserve(out.size());
        QRegularExpressionMatch m;
        // clang-format off
        for (const auto &o : out) {
            if (ref & Head && (m = headRe.match(o)).hasMatch()) {
                branches.append({m.captured(1),
                                 QString(), // no remote
                                 m.captured(2),
                                 RefType::Head,
                                -1});
            } else if (ref & Remote && (m = remoteRe.match(o)).hasMatch()) {
                branches.append({m.captured(1).append(QLatin1Char('/') + m.captured(2)),
                                 m.captured(1),
                                 m.captured(3),
                                 RefType::Remote,
                                -1});
            } else if (ref & Tag && (m = tagRe.match(o)).hasMatch()) {
                branches.append({m.captured(1),
                                 QString(), // no remote
                                 m.captured(3).isEmpty() ? QString() : m.captured(2),
                                 RefType::Tag,
                                -1});
            }
        }
        // clang-format on
    }

    return branches;
}
