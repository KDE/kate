/*
    SPDX-FileCopyrightText: 2022 Jiří Wolker <woljiri@gmail.com>
    SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "recentitemsmodel.h"

#include "ktexteditor_utils.h"

#include <QMimeDatabase>

RecentItemsModel::RecentItemsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant RecentItemsModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const size_t row = index.row();
        if (row < m_recentItems.size()) {
            const RecentItemInfo &recentItem = m_recentItems.at(row);
            switch (role) {
            case Qt::DisplayRole:
                return recentItem.name;
            case Qt::DecorationRole:
                return recentItem.icon;
            case Qt::ToolTipRole:
                return recentItem.url.toString(QUrl::PreferLocalFile);
            default:
                break;
            }
        }
    }

    return {};
}

int RecentItemsModel::rowCount(const QModelIndex &) const
{
    return int(m_recentItems.size());
}

void RecentItemsModel::refresh(const QList<QUrl> &urls)
{
    std::vector<RecentItemInfo> recentItems;
    recentItems.reserve(urls.size());

    QIcon icon;
    QString name;
    for (const QUrl &url : urls) {
        // lookup mime type without accessing file to avoid stall for e.g. NFS/SMB
        const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(url.path(), QMimeDatabase::MatchExtension);
        if (url.isLocalFile() || !mimeType.isDefault()) {
            icon = QIcon::fromTheme(mimeType.iconName());
        } else {
            icon = QIcon::fromTheme(QStringLiteral("network-server"));
        }

        // we want some filename @ folder output to have chance to keep important stuff even on elide, see bug 472981
        name = Utils::niceFileNameWithPath(url);

        recentItems.push_back({icon, name, url});
    }

    beginResetModel();
    m_recentItems = std::move(recentItems);
    endResetModel();
}

QUrl RecentItemsModel::url(const QModelIndex &index) const
{
    if (index.isValid()) {
        const size_t row = index.row();
        if (row < m_recentItems.size()) {
            return m_recentItems.at(row).url;
        }
    }

    return {};
}
