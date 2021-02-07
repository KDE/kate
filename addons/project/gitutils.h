#ifndef GITUTILS_H
#define GITUTILS_H

#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QString>

namespace GitUtils
{
// clang-format off
enum RefType {
    Head   = 0x1,
    Remote = 0x2,
    Tag    = 0x4,
    All    = 0x7
};
// clang-format on

struct Branch {
    QString name;
    QString remote;
    QString commit;
    RefType type;
};

bool isGitRepo(const QString &repo);

QString getCurrentBranchName(const QString &repo);

int checkoutBranch(const QString &repo, const QString &branch);

QVector<Branch> getAllBranches(const QString &repo, RefType ref = RefType::All);
}

#endif // GITUTILS_H
