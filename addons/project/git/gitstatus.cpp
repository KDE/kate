/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "gitstatus.h"

#include <KLocalizedString>
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
            untracked.append({QString::fromUtf8(file, size), GitStatus::Untracked, 'U'});
            break;
        case StatusXY::II:
            untracked.append({QString::fromUtf8(file, size), GitStatus::Ignored, 'I'});
            break;

        case StatusXY::DD:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_BothDeleted, x});
            break;
        case StatusXY::AU:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_AddedByUs, x});
            break;
        case StatusXY::UD:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_DeletedByThem, x});
            break;
        case StatusXY::UA:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_AddedByThem, x});
            break;
        case StatusXY::DU:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_DeletedByUs, x});
            break;
        case StatusXY::AA:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_BothAdded, x});
            break;
        case StatusXY::UU:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_BothModified, x});
            break;
        }

        switch (x) {
        case 'M':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Modified, x});
            break;
        case 'A':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Added, x});
            break;
        case 'D':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Deleted, x});
            break;
        case 'R':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Renamed, x});
            break;
        case 'C':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Copied, x});
            break;
        }

        switch (y) {
        case 'M':
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_Modified, y});
            break;
        case 'D':
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_Deleted, y});
            break;
        case 'A':
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_IntentToAdd, y});
            break;
        }
    }

    return {untracked, unmerge, staged, changed};
}

QString GitUtils::statusString(GitUtils::GitStatus s)
{
    switch (s) {
    case WorkingTree_Modified:
    case Index_Modified:
        return i18n(" ‣ Modified");
    case Untracked:
        return i18n(" ‣ Untracked");
    case Index_Renamed:
        return i18n(" ‣ Renamed");
    case Index_Deleted:
    case WorkingTree_Deleted:
        return i18n(" ‣ Deleted");
    case Index_Added:
    case WorkingTree_IntentToAdd:
        return i18n(" ‣ Added");
    case Index_Copied:
        return i18n(" ‣ Copied");
    case Ignored:
        return i18n(" ‣ Ignored");
    case Unmerge_AddedByThem:
    case Unmerge_AddedByUs:
    case Unmerge_BothAdded:
    case Unmerge_BothDeleted:
    case Unmerge_BothModified:
    case Unmerge_DeletedByThem:
    case Unmerge_DeletedByUs:
        return i18n(" ‣ Conflicted");
    }
    return QString();
}
