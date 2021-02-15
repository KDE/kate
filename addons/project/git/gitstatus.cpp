#include "gitstatus.h"

#include <QByteArray>
#include <QList>

GitUtils::GitParsedStatus GitUtils::parseStatus(const QByteArray &raw)
{
    QVector<GitUtils::StatusItem> untracked;
    QVector<GitUtils::StatusItem> unmerge;
    QVector<GitUtils::StatusItem> staged;
    QVector<GitUtils::StatusItem> changed;

    QList<QByteArray> rawList = raw.split(0x00);
    for (const auto &r : rawList) {
        if (r.isEmpty() || r.length() < 3) {
            continue;
        }

        char x = r.at(0);
        char y = r.at(1);
        uint16_t xy = (((uint16_t)x) << 8) | y;
        using namespace GitUtils;

        const char *file = r.data() + 3;
        const int size = r.size() - 3;

        switch (xy) {
        case StatusXY::QQ:
            untracked.append({QString::fromUtf8(file, size), GitStatus::Untracked});
            break;
        case StatusXY::II:
            untracked.append({QString::fromUtf8(file, size), GitStatus::Ignored});
            break;

        case StatusXY::DD:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_BothDeleted});
            break;
        case StatusXY::AU:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_AddedByUs});
            break;
        case StatusXY::UD:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_DeletedByThem});
            break;
        case StatusXY::UA:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_AddedByThem});
            break;
        case StatusXY::DU:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_DeletedByUs});
            break;
        case StatusXY::AA:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_BothAdded});
            break;
        case StatusXY::UU:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_BothModified});
            break;
        }

        switch (x) {
        case 'M':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Modified});
            break;
        case 'A':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Added});
            break;
        case 'D':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Deleted});
            break;
        case 'R':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Renamed});
            break;
        case 'C':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Copied});
            break;
        }

        switch (y) {
        case 'M':
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_Modified});
            break;
        case 'D':
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_Deleted});
            break;
        case 'A':
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_IntentToAdd});
            break;
        }
    }

    return {untracked, unmerge, staged, changed};
}
