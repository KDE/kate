/*
    SPDX-FileCopyrightText: 2025 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "bookmarksmodel.h"

#include <KLocalizedString>
#include <QIcon>
#include <QUrl>

BookmarksModel::BookmarksModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

BookmarksModel::~BookmarksModel() = default;

int BookmarksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_bookmarks.size();
}

int BookmarksModel::columnCount(const QModelIndex &) const
{
    return 2;
}

QVariant BookmarksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_bookmarks.size()) {
        return QVariant();
    }

    const Bookmark &bookmark = m_bookmarks.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case LineNumber:
            return bookmark.lineNumber + 1;
        case FilePath:
            return bookmark.url.path();
        default:
            return QVariant();
        }
    }

    if (role == Qt::DecorationRole && index.column() == 0) {
        static QIcon icon = QIcon::fromTheme(QStringLiteral("bookmarks"));
        return icon;
    }

    return QVariant();
}

QVariant BookmarksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case LineNumber:
            return i18n("Line Number");
        case FilePath:
            return i18n("File Path");
        default:
            return QVariant();
        }
    }

    return QVariant();
}

bool BookmarksModel::bookmarkLinesMatch(const Range &range, const QList<int> &lineNumbers) const
{
    if (range.count != lineNumbers.size()) {
        return false;
    }

    for (int i = 0; i < range.count; ++i) {
        if (m_bookmarks[range.start + i].lineNumber != lineNumbers[i]) {
            return false;
        }
    }

    return true;
}

const Bookmark &BookmarksModel::getBookmark(const QModelIndex &index)
{
    return m_bookmarks.at(index.row());
}

void BookmarksModel::setBookmarks(const QUrl &url, const QList<int> &lineNumbers)
{
    int nMarks = lineNumbers.size();
    auto rangeIt = m_bookmarksIndexes.find(url);

    if (rangeIt != m_bookmarksIndexes.end()) {
        auto range = rangeIt.value();

        // Skip update if bookmarks are not changed
        if (bookmarkLinesMatch(range, lineNumbers)) {
            return;
        }

        int rangeEnd = range.start + range.count;
        beginRemoveRows(QModelIndex(), range.start, rangeEnd - 1);
        m_bookmarks.erase(m_bookmarks.begin() + range.start, m_bookmarks.begin() + rangeEnd);
        endRemoveRows();

        // Update ranges after removed url
        m_bookmarksIndexes.remove(url);
        for (auto it = m_bookmarksIndexes.begin(); it != m_bookmarksIndexes.end(); ++it) {
            auto &otherRange = it.value();
            if (otherRange.start > range.start) {
                otherRange.start -= range.count;
            }
        }
    }

    if (nMarks > 0) {
        // Insert new block at the end
        int insertPos = m_bookmarks.size();
        m_bookmarksIndexes.insert(url, {insertPos, nMarks});
        beginInsertRows(QModelIndex(), insertPos, insertPos + nMarks - 1);
        for (int line : lineNumbers) {
            m_bookmarks.append({url, line});
        }

        endInsertRows();
    }
}

QModelIndex BookmarksModel::getBookmarkIndex(const Bookmark &bookmark)
{
    auto rangeIt = m_bookmarksIndexes.find(bookmark.url);
    if (rangeIt == m_bookmarksIndexes.end()) {
        return QModelIndex();
    }

    auto range = rangeIt.value();
    for (int i = 0; i < range.count; ++i) {
        const Bookmark &bookmarkEntry = m_bookmarks[range.start + i];
        if (bookmarkEntry.lineNumber == bookmark.lineNumber) {
            return index(range.start + i, 0);
        }
    }

    return QModelIndex();
}

#include "moc_bookmarksmodel.cpp"
