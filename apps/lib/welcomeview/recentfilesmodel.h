/*
    SPDX-FileCopyrightText: 2022 Jiří Wolker <woljiri@gmail.com>
    SPDX-FileCopyrightText: 2022 Eugene Popov <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef RECENTFILESMODEL_H
#define RECENTFILESMODEL_H

#include <QAbstractListModel>
#include <QIcon>
#include <QUrl>

class RecentFilesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit RecentFilesModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    void refresh(const QList<QUrl> &urls);
    QUrl url(const QModelIndex &index) const;

private:
    struct RecentFileInfo {
        QIcon icon;
        QString name;
        QUrl url;
    };

    QVector<RecentFileInfo> m_recentFiles;
};

#endif // RECENTFILESMODEL_H
