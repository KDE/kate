/* This file is part of the KDE project

   SPDX-FileCopyrightText: 2018 Gregor Mi <codestruct@posteo.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QAbstractTableModel>
#include <QIcon>
#include <QString>

#include <doc_or_widget.h>

namespace KTextEditor
{
class Document;
}

namespace detail
{
/**
 * Represents one item in the table view of the tab switcher.
 */
class FilenameListItem
{
public:
    explicit FilenameListItem(DocOrWidget doc);

    DocOrWidget document;
    QIcon icon() const;
    QString documentName() const;
    QString fullPath() const;

    /**
     * calculated from documentName and fullPath
     */
    QString displayPathPrefix;
};
using FilenameList = std::vector<FilenameListItem>;

class TabswitcherFilesModel : public QAbstractTableModel
{
public:
    explicit TabswitcherFilesModel(QObject *parent = nullptr);
    ~TabswitcherFilesModel() override = default;
    bool insertDocuments(int row, const QList<DocOrWidget> &document);
    bool removeDocument(DocOrWidget document);

    /**
     * Clears all data from the model
     */
    void clear();

    /**
     * NOTE: The returned pointer will become invalid as soon as the underlying vector changes.
     */
    DocOrWidget item(int row) const;

    /**
     * Move the document to row 0.
     */
    void raiseDocument(DocOrWidget document);

    /*
     * Use this method to update all items.
     * This is typically needed when a document name changes, since then the prefix paths change,
     * so all items need an update.
     */
    void updateItems();

    /**
     * Reimplemented to return the column count of top-level items.
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Reimplemented to return the top-level row count.
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Returns the data for the requested model index.
     */
    QVariant data(const QModelIndex &index, int role) const override;

    /**
     * Reimplemented to remove the specified rows.
     * The parent is always ignored since this is a table model.
     */
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

private:
    FilenameList data_;
};

QString longestCommonPrefix(std::vector<QString> const &strs);

}
