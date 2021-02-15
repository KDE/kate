#ifndef GITSTATUS_H
#define GITSTATUS_H

#include <QString>
#include <QVector>

namespace GitUtils
{
enum GitStatus {
    Unmerge_BothDeleted,
    Unmerge_AddedByUs,
    Unmerge_DeletedByThem,
    Unmerge_AddedByThem,
    Unmerge_DeletedByUs,
    Unmerge_BothAdded,
    Unmerge_BothModified,

    Index_Modified,
    Index_Added,
    Index_Deleted,
    Index_Renamed,
    Index_Copied,

    WorkingTree_Modified,
    WorkingTree_Deleted,
    WorkingTree_IntentToAdd,

    IndexWorkingTree_Modified,

    Untracked,
    Ignored
};

enum StatusXY {
    DD = 0x4444,
    AU = 0x4155,
    UD = 0x5544,
    UA = 0x5541,
    DU = 0x4455,
    AA = 0x4141,
    UU = 0x5555,

    /* underscore represents space */
    //    M_ = 0x4d20,
    //    A_ = 0x4120,
    //    D_ = 0x4420,
    //    R_ = 0x5220,
    //    C_ = 0x4320,

    //    _M = 0x204d,
    //    _D = 0x2044,
    //    _A = 0x2041,

    //??
    QQ = 0x3f3f,
    //!!
    II = 0x2121,
};

struct StatusItem {
    QString file;
    GitStatus status;
};

struct GitParsedStatus {
    QVector<StatusItem> untracked;
    QVector<StatusItem> unmerge;
    QVector<StatusItem> staged;
    QVector<StatusItem> changed;
};

GitParsedStatus parseStatus(const QByteArray &raw);
}

#endif // GITSTATUS_H
