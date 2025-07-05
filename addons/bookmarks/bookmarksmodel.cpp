/*
    SPDX-FileCopyrightText: 2025 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "bookmarksmodel.h"

#include <KLocalizedString>
#include <QIcon>
#include <QUrl>

BookmarksModel::BookmarksModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

BookmarksModel::~BookmarksModel() = default;

QModelIndex BookmarksModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex BookmarksModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

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
        case 0:
            return bookmark.lineNumber + 1;
        case 1:
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
        case 0:
            return i18n("Line Number");
        case 1:
            return i18n("File Path");
        default:
            return QVariant();
        }
    }

    return QVariant();
}

const Bookmark &BookmarksModel::getBookmark(const QModelIndex &index)
{
    return m_bookmarks.at(index.row());
}

void BookmarksModel::setBookmarks(const QUrl &url, const QList<int> &lineNumbers)
{
    int nMarks = lineNumbers.size();
    auto it = m_bookmarksIndexes.find(url);

    if (it != m_bookmarksIndexes.end()) {
        int start = it.value().first;
        int count = it.value().second;

        // Skip update if bookmarks are not changed
        if (count == nMarks) {
            bool equals = true;
            for (int i = start; i < start + count; ++i) {
                if (m_bookmarks[i].lineNumber != lineNumbers[i - start]) {
                    equals = false;
                    break;
                }
            }
            if (equals) {
                return;
            }
        }

        beginRemoveRows(QModelIndex(), start, start + count - 1);
        m_bookmarks.erase(m_bookmarks.begin() + start, m_bookmarks.begin() + start + count);
        endRemoveRows();

        // Update ranges after removed url
        m_bookmarksIndexes.remove(url);
        for (auto it2 = m_bookmarksIndexes.begin(); it2 != m_bookmarksIndexes.end(); ++it2) {
            if (it2.value().first > start) {
                it2.value().first -= count;
            }
        }
    }

    if (nMarks > 0) {
        // Insert new block at the end
        int insertPos = m_bookmarks.size();
        m_bookmarksIndexes.insert(url, qMakePair(insertPos, nMarks));
        beginInsertRows(QModelIndex(), insertPos, insertPos + nMarks - 1);
        for (int line : lineNumbers) {
            m_bookmarks.append({url, line});
        }

        endInsertRows();
    }
}
