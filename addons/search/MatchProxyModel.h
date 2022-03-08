/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_SEARCH_MATCH_PROXY_MODEL_H
#define KATE_SEARCH_MATCH_PROXY_MODEL_H

#include <QSortFilterProxyModel>

class MatchProxyModel final : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    bool filterAcceptsRow(int sourceRow, const QModelIndex &parent) const override;

    Q_SLOT void setFilterText(const QString &text);

};

#endif
