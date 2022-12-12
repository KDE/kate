/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katefiletreeproxymodel.h"
#include "katefiletreedebug.h"
#include "katefiletreemodel.h"

#include <QCollator>
#include <ktexteditor/document.h>

KateFileTreeProxyModel::KateFileTreeProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void KateFileTreeProxyModel::setSourceModel(QAbstractItemModel *model)
{
    Q_ASSERT(qobject_cast<KateFileTreeModel *>(model)); // we don't really work with anything else
    QSortFilterProxyModel::setSourceModel(model);
}

KTextEditor::Document *KateFileTreeProxyModel::docFromIndex(const QModelIndex &index)
{
    return data(index, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
}

QList<KTextEditor::Document *> KateFileTreeProxyModel::docTreeFromIndex(const QModelIndex &index)
{
    return data(index, KateFileTreeModel::DocumentTreeRole).value<QList<KTextEditor::Document *>>();
}

bool KateFileTreeProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const KateFileTreeModel *model = static_cast<KateFileTreeModel *>(sourceModel());

    const bool left_isdir = model->isDir(left);
    const bool right_isdir = model->isDir(right);

    // in tree mode, there will be parent nodes, we want to put those first
    if (left_isdir != right_isdir) {
        return ((left_isdir - right_isdir)) > 0;
    }

    QCollator collate;
    collate.setCaseSensitivity(Qt::CaseInsensitive);
    collate.setNumericMode(true);

    switch (sortRole()) {
    case Qt::DisplayRole: {
        const QString left_name = model->data(left).toString();
        const QString right_name = model->data(right).toString();
        return collate.compare(left_name, right_name) < 0;
    }

    case KateFileTreeModel::PathRole: {
        const auto l = model->data(left, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
        const auto r = model->data(right, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
        if (l && r) {
            return l->url() < r->url();
        }
        return false;
    }

    case KateFileTreeModel::OpeningOrderRole:
    case CustomSorting:
        return (left.row() - right.row()) < 0;
    }

    return false;
}

QModelIndex KateFileTreeProxyModel::docIndex(const KTextEditor::Document *doc) const
{
    return mapFromSource(static_cast<KateFileTreeModel *>(sourceModel())->docIndex(doc));
}

bool KateFileTreeProxyModel::isDir(const QModelIndex &index) const
{
    return static_cast<KateFileTreeModel *>(sourceModel())->isDir(mapToSource(index));
}

QModelIndex KateFileTreeProxyModel::widgetIndex(QWidget *w) const
{
    return mapFromSource(static_cast<KateFileTreeModel *>(sourceModel())->widgetIndex(w));
}

bool KateFileTreeProxyModel::isWidgetDir(const QModelIndex &i) const
{
    return static_cast<KateFileTreeModel *>(sourceModel())->isWidgetDir(mapToSource(i));
}

bool KateFileTreeProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    // Child rows accepted as is
    if (source_parent.isValid()) {
        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
    }

    const auto index = sourceModel()->index(source_row, 0, source_parent);
    if (static_cast<KateFileTreeModel *>(sourceModel())->isWidgetDir(index)) {
        return sourceModel()->rowCount(index) > 0;
    }
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
