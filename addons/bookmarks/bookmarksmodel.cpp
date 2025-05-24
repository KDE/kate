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

QModelIndex BookmarksModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int BookmarksModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return parent.isValid() ? 0 : m_keys.size();
}

int BookmarksModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant BookmarksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_keys.size())
        return QVariant();

    const Bookmark &bookmark = m_bookmarks[m_keys.at(index.row())];
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

int BookmarksModel::getBookmarkId(const QUrl &url, int lineNumber)
{
    return std::hash<QString>()(url.toString()) ^ std::hash<int>()(lineNumber);
}

void BookmarksModel::addBookmark(const QUrl &url, int lineNumber)
{
    int id = getBookmarkId(url, lineNumber);
    if (m_bookmarks.contains(id))
        return;

    beginInsertRows(QModelIndex(), m_keys.size(), m_keys.size());
    m_bookmarks.insert(id, {url, lineNumber});
    m_keys.append(id);
    endInsertRows();
}

void BookmarksModel::removeBookmark(const QUrl &url, int lineNumber)
{
    int id = getBookmarkId(url, lineNumber);
    if (!m_bookmarks.contains(id))
        return;

    int row = m_keys.indexOf(id);
    if (row == -1)
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_bookmarks.remove(id);
    m_keys.removeAt(row);
    endRemoveRows();
}

const Bookmark &BookmarksModel::getBookmark(const QModelIndex &index)
{
    return m_bookmarks[m_keys.at(index.row())];
}
