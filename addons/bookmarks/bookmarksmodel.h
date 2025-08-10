/*
    SPDX-FileCopyrightText: 2025 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QString>
#include <QUrl>

struct Bookmark {
    QUrl url;
    int lineNumber;
};

class BookmarksModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit BookmarksModel(QObject *parent = nullptr);
    ~BookmarksModel() override;

    // Required model interface methods
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Bookmarks operations
    void setBookmarks(const QUrl &url, const QList<int> &lineNumbers);
    const Bookmark &getBookmark(const QModelIndex &index);
    QModelIndex getBookmarkIndex(const Bookmark &bookmark);

private:
    struct Range {
        int start = 0;
        int count = 0;
    };

    QList<Bookmark> m_bookmarks;
    QHash<QUrl, Range> m_bookmarksIndexes;

private:
    bool bookmarkLinesMatch(const Range &range, const QList<int> &lineNumbers) const;
};