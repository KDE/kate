/*
    SPDX-FileCopyrightText: 2025 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <QUrl>

struct Bookmark {
    QUrl url;
    int lineNumber;
};

class BookmarksModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit BookmarksModel(QObject *parent = nullptr);
    ~BookmarksModel() override;

    // Required model interface methods
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Bookmarks operations
    void setBookmarks(const QUrl &url, const QList<int> &lineNumbers);
    const Bookmark &getBookmark(const QModelIndex &index);

private:
    QList<Bookmark> m_bookmarks;
    QHash<QUrl, QPair<int, int>> m_bookmarksIndexes;
};
