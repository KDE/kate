/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "snippetfilterproxymodel.h"

#include "snippetstore.h"
#include "snippet.h"

SnippetFilterProxyModel::SnippetFilterProxyModel(QObject *parent)
 : QSortFilterProxyModel(parent)
{
    connect(SnippetStore::self(),
            SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this,
            SLOT(dataChanged(QModelIndex,QModelIndex)));
}


SnippetFilterProxyModel::~SnippetFilterProxyModel()
{
}

QVariant SnippetFilterProxyModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole && index.parent().isValid()) {
        // in the view, also show prefix, postfix and arguments
        Snippet* snippet = dynamic_cast<Snippet*>( SnippetStore::self()->itemFromIndex(mapToSource(index)) );
        if (snippet) {
            QString ret = snippet->prefix() + ' ' + snippet->text() + snippet->arguments() + ' ' + snippet->postfix();
            return ret.trimmed();
        }
    }
    return QSortFilterProxyModel::data(index, role);
}

void SnippetFilterProxyModel::changeFilter(const QString& filter)
{
    filter_ = filter;
    clear();
}

bool SnippetFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const
{
    if (filter_.isEmpty()) {
        // No filtering needed...
        return true;
    }

    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    QStandardItem* item = SnippetStore::self()->itemFromIndex( index );
    if (!item)
        return false;

    Snippet* snippet = dynamic_cast<Snippet*>( item );
    if (snippet) {
        if ( snippet->text().contains( filter_) )
            return true;
        else
            return false;
    }

    // if it's not a snippet; allow it...
    return true;
}

void SnippetFilterProxyModel::dataChanged(const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/)
{
    // If we don't do this, the model will contain strange QModelIndex elements after a
    // sync of a repository. Stangely this only happens on Linux. When running under Windows
    // everything's ok, evan without this hack.
    // By letting the proxy reevaluate the items, these elements will be removed.

    ///@todo check if this is OK
    clear();
}

#include "snippetfilterproxymodel.moc"
