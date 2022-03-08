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

    return matches;
}
