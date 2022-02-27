/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "MatchProxyModel.h"
#include "MatchModel.h"

void MatchProxyModel::setFilterText(const QString &text)
{
    beginResetModel();
    auto *matchModel = dynamic_cast<MatchModel *>(sourceModel());
    matchModel->setFilterText(text);
    endResetModel();
}

bool MatchProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &parent) const
{
    // root item always visible
    if (!parent.isValid()) {
        return true;
    }

    const auto index = sourceModel()->index(sourceRow, 0, parent);
    if (!index.isValid()) {
        return false;
    }

    // match text;
    auto *matchModel = dynamic_cast<MatchModel *>(sourceModel());
    bool matches = matchModel->matchesFilter(index);

    if (matches) {
        return true;
    }

    // text didn't match. Check if this is a match item & its parent is accepted
    if (isMatchItem(index) && parentAcceptsRow(parent)) {
        return true;
    }

    // filter it out
    return false;
}

bool MatchProxyModel::isMatchItem(const QModelIndex &index)
{
    return index.parent().isValid() && index.parent().parent().isValid();
}

bool MatchProxyModel::parentAcceptsRow(const QModelIndex &source_parent) const
{
    if (source_parent.isValid()) {
        const QModelIndex index = source_parent.parent();
        if (filterAcceptsRow(source_parent.row(), index)) {
            return true;
        }
        // we don't want to recurse because our root item is always accepted
    }
    return false;
}
