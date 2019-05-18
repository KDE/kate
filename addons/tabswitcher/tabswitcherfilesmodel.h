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

#ifndef KTEXTEDITOR_TAB_SWITCHER_FILES_MODEL_H
#define KTEXTEDITOR_TAB_SWITCHER_FILES_MODEL_H

#include <QString>
#include <QIcon>
#include <QAbstractTableModel>

namespace KTextEditor {
    class Document;
}

namespace detail {

/**
 * Represents one item in the table view of the tab switcher.
 */
class FilenameListItem
{
public:
    FilenameListItem(KTextEditor::Document* doc);

    KTextEditor::Document *document;
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
    bool insertRow(int row, const FilenameListItem & item);
    bool removeRow(int row);
    /**
     * Clears all data from the model
     */
    void clear();
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

    /**
     * Reimplemented to return the column count of top-level items.
     */
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;

    /**
     * Reimplemented to return the top-level row count.
     */
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;

    /**
     * Returns the data for the requested model index.
     */
    QVariant data(const QModelIndex & index, int role) const override;

private:
    FilenameList data_;
};

QString longestCommonPrefix(std::vector<QString> const & strs);

}

#endif // KTEXTEDITOR_TAB_SWITCHER_FILES_MODEL_H
