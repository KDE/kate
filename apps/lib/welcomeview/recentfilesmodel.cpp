/*
    SPDX-FileCopyrightText: 2022 Jiří Wolker <woljiri@gmail.com>
    SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "recentfilesmodel.h"

#include "kateconfigdialog.h"

#include <QFileInfo>
#include <QMimeDatabase>

RecentFilesModel::RecentFilesModel(QObject *parent)
    : QAbstractListModel(parent)
{}

QVariant RecentFilesModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const int row = index.row();
        if (row >= 0 && row < m_recentFiles.count()) {
            const RecentFileInfo &recentFile = m_recentFiles.at(row);
            switch (role) {
            case Qt::DisplayRole:
                return recentFile.name;
            case Qt::DecorationRole:
                return recentFile.icon;
            case Qt::ToolTipRole:
                return recentFile.url.toString(QUrl::PreferLocalFile);
            default:
                break;
            }
        }
    }

    return QVariant();
}

int RecentFilesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_recentFiles.count();
}

void RecentFilesModel::refresh(const QList<QUrl> &urls)
{
    QVector<RecentFileInfo> recentFiles;
    recentFiles.reserve(urls.count());

    QIcon icon;
    QString name;
    for (const QUrl &url: urls) {
        if (url.isLocalFile()) {
            const QFileInfo fileInfo(url.toLocalFile());
            if (!fileInfo.exists()) {
                continue;
            }

            icon = QIcon::fromTheme(QMimeDatabase().mimeTypeForFile(fileInfo).iconName());
            name = fileInfo.fileName();
        } else {
            icon = QIcon::fromTheme(QStringLiteral("network-server"));
            name = url.toString();
        }

        recentFiles.append({ icon, name, url });
    }

    beginResetModel();
    m_recentFiles = std::move(recentFiles);
    endResetModel();
}

QUrl RecentFilesModel::url(const QModelIndex &index) const
{
    if (index.isValid()) {
        const int row = index.row();
        if (row >= 0 && row < m_recentFiles.count()) {
            return m_recentFiles.at(row).url;
        }
    }

    return QUrl();
}
