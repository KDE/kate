/* This file is part of the KDE project

   Copyright (C) 2018 Gregor Mi <codestruct@posteo.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "tabswitcherfilesmodel.h"

#include <QDebug>
#include <QBrush>
#include <QFileInfo>
#include <QMimeDatabase>

#include <KTextEditor/Document>

#include <algorithm>

namespace detail
{
    static QIcon iconForDocument(KTextEditor::Document * doc)
    {
        return QIcon::fromTheme(QMimeDatabase().mimeTypeForUrl(doc->url()).iconName());
    }

    FilenameListItem::FilenameListItem(KTextEditor::Document* doc)
        : document(doc)
        , icon(iconForDocument(doc))
        , documentName(doc->documentName())
        , fullPath(doc->url().toLocalFile())
    {
    }

    /**
     * adapted from https://helloacm.com/c-coding-exercise-longest-common-prefix/
     * see also http://www.cplusplus.com/forum/beginner/83540/
     * Note that if strs contains the empty string, the result will be ""
     */
    QString longestCommonPrefix(std::vector<QString> const & strs) {
        int n = INT_MAX;
        if (strs.size() <= 0) {
            return QString();
        }
        if (strs.size() == 1) {
            return strs[0];
        }
        // get the min length
        for (size_t i = 0; i < strs.size(); i++) {
            n = strs[i].length() < n ? strs[i].length() : n;
        }
        for (int c = 0; c < n; c++) { // check each character
            for (size_t i = 1; i < strs.size(); i++) {
                if (strs[i][c] != strs[i - 1][c]) { // we find a mis-match
                    return QStringRef(&strs[0], 0, c).toString();
                }
            }
        }
        // prefix is n-length
        return QStringRef(&strs[0], 0, n).toString();
    }

    void post_process(FilenameList & data)
    {
        std::vector<QString> paths;
        std::for_each(data.begin(), data.end(),
                    [&paths](FilenameListItem & item) { paths.push_back(item.fullPath); });

        // Removes empty strings because Documents without file have no path and we would
        // otherwise in this case always get ""
        paths.erase( // erase-remove idiom, see https://en.cppreference.com/w/cpp/algorithm/remove
            std::remove_if(paths.begin(), paths.end(), [](const QString &s) {
                return s.isEmpty(); }),
            paths.end()
        );
        QString prefix = longestCommonPrefix(paths);
        int prefix_length = prefix.length();
        if (prefix_length == 1) { // if there is only the "/" at the beginning, then keep it
            prefix_length = 0;
        }

        std::for_each(data.begin(), data.end(),
                        [prefix_length](FilenameListItem & item) {
                            // Note that item.documentName can contain additional characters - e.g. "README.md (2)" -
                            // so we cannot use that and have to parse the base filename by other means:
                            QFileInfo fileinfo(item.fullPath);
                            QString basename = fileinfo.fileName(); // e.g. "archive.tar.gz"
                            // cut prefix (left side) and cut document name (plus slash) on the right side
                            int len = item.fullPath.length() - prefix_length - basename.length() - 1;
                            if (len > 0) { // only assign in case item.fullPath is not empty
                                // "PREFIXPATH/REMAININGPATH/BASENAME" --> "REMAININGPATH"
                                item.displayPathPrefix = QStringRef(&item.fullPath, prefix_length, len).toString();
                            }
                        });
    }
}

detail::TabswitcherFilesModel::TabswitcherFilesModel(QObject *parent) : QAbstractTableModel(parent)
{
}

detail::TabswitcherFilesModel::TabswitcherFilesModel(const FilenameList & data)
{
    data_ = data;
    post_process(data_);
}

bool detail::TabswitcherFilesModel::insertRow(int row, const FilenameListItem & item)
{
    beginInsertRows(QModelIndex(), row, row + 1);
    data_.insert(data_.begin() + row, item);
    post_process(data_);
    endInsertRows();
    return true;
}

bool detail::TabswitcherFilesModel::removeRow(int row)
{
    if (row < 0 || row >= rowCount()) {
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row);
    data_.erase(data_.begin() + row);
    post_process(data_);
    endRemoveRows();
    return true;
}

void detail::TabswitcherFilesModel::clear()
{
    if (data_.size() > 0) {
        beginRemoveRows(QModelIndex(), 0, data_.size() - 1);
        data_.clear();
        endRemoveRows();
    }
}

detail::FilenameListItem * detail::TabswitcherFilesModel::item(int row) const
{
    return const_cast<detail::FilenameListItem *>(&data_[row]);
}

void detail::TabswitcherFilesModel::updateItem(FilenameListItem * item, QString const & documentName, QString const & fullPath)
{
    item->documentName = documentName;
    item->fullPath = fullPath;
    post_process(data_);
}

int detail::TabswitcherFilesModel::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return 2;
}

int detail::TabswitcherFilesModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return data_.size();
}

QVariant detail::TabswitcherFilesModel::data(const QModelIndex & index, int role) const
{
    if (role == Qt::DisplayRole) {
        const auto & row = data_[index.row()];
        if (index.column() == 0)
            return row.displayPathPrefix;
        else
            return row.documentName;
    } else if (role == Qt::DecorationRole) {
        if (index.column() == 1) {
            const auto & row = data_[index.row()];
            return row.icon;
        }
    } else if (role == Qt::ToolTipRole) {
        const auto & row = data_[index.row()];
        return row.fullPath;
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 0)
            return Qt::AlignRight + Qt::AlignVCenter;
        else
            return Qt::AlignVCenter;
    } else if (role == Qt::ForegroundRole) {
        if (index.column() == 0)
            return QBrush(Qt::darkGray);
        else
            return QVariant();
    }
    return QVariant();
}
