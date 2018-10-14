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

#pragma once

#include <QString>
#include <QIcon>
#include <QStandardItem>

namespace detail {

/**
 * The implementation is close to QStandardItem but not all aspects are supported.
 * Probably it would be better not to derive from QStandardItem.
 */
struct FilenameListItem : public QStandardItem
{
    FilenameListItem(const QIcon & icon, const QString & documentName, const QString & fullPath)
    {
        this->icon = icon;
        this->documentName = documentName;
        this->fullPath = fullPath;
    }

    QIcon icon;
    QString documentName;
    QString fullPath;
    /**
     * calculated from documentName and fullPath
     */
    QString displayPathPrefix;
};
using FilenameList = std::vector<FilenameListItem>;

class TabswitcherFilesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TabswitcherFilesModel(QObject *parent = nullptr);
    virtual ~TabswitcherFilesModel() = default;
    TabswitcherFilesModel(const FilenameList & data);
    bool insertRow(int row, FilenameListItem const * const item);
    bool removeRow(int row);
    int rowCount() const;
    /**
     * NOTE: The returned pointer will become invalid as soon as the underlying vector changes.
     */
    FilenameListItem * item(int row) const;
    /*
     * Use this method to update an item.
     * NOTE: This could be improved if we allow KTextEditor::Document to go into this interface.
     * Then we could search and update by KTextEditor::Document.
     */
    void updateItem(FilenameListItem * item, QString const & documentName, QString const & fullPath);

protected:
    int columnCount(const QModelIndex & parent) const override;
    int rowCount(const QModelIndex & parent) const override;
    QVariant data(const QModelIndex & index, int role) const override;

private:
    FilenameList data_;
};

QString longestCommonPrefix(std::vector<QString> const & strs);

}
