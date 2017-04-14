/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

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

#include "katefiletreeproxymodel.h"
#include "katefiletreemodel.h"
#include "katefiletreedebug.h"

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
        const QString left_name = model->data(left, KateFileTreeModel::PathRole).toString();
        const QString right_name = model->data(right, KateFileTreeModel::PathRole).toString();
        return collate.compare(left_name, right_name) < 0;
    }

    case KateFileTreeModel::OpeningOrderRole:
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

