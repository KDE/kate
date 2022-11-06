/*
    SPDX-FileCopyrightText: 2022 Jiří Wolker <woljiri@gmail.com>
    SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "recentitemsmodel.h"

#include <QMimeDatabase>

RecentItemsModel::RecentItemsModel(QObject *parent)
    : QAbstractListModel(parent)
{}

QVariant RecentItemsModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const int row = index.row();
        if (row >= 0 && row < m_recentItems.count()) {
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

    return QVariant();
}

int RecentItemsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_recentItems.count();
}

void RecentItemsModel::refresh(const QList<QUrl> &urls)
{
    QVector<RecentItemInfo> recentItems;
    recentItems.reserve(urls.count());

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

        if (url.isLocalFile()) {
            name = url.fileName();
        } else {
            name = url.toDisplayString();
        }

        recentItems.append({icon, name, url});
    }

    beginResetModel();
    m_recentItems = std::move(recentItems);
    endResetModel();
}

QUrl RecentItemsModel::url(const QModelIndex &index) const
{
    if (index.isValid()) {
        const int row = index.row();
        if (row >= 0 && row < m_recentItems.count()) {
            return m_recentItems.at(row).url;
        }
    }

    return QUrl();
}
