/***************************************************************************
 *   This file is part of Kate build plugin                                *
 *   SPDX-FileCopyrightText: 2022 Kåre Särs <kare.sars@iki.fi>             *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 ***************************************************************************/
#include "TargetFilterProxyModel.h"

#include <QDebug>

TargetFilterProxyModel::TargetFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool TargetFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_filter.isEmpty()) {
        return true;
    }

    QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
    QString name = index0.data().toString();

    if (index0.internalId() == 0xffffffff) {
        int i = 0;
        auto childIndex = index0.model()->index(i, 0, index0);
        while (childIndex.data().isValid()) {
            name = childIndex.data().toString();
            if (name.contains(m_filter, Qt::CaseInsensitive)) {
                return true;
            }
            i++;
            childIndex = index0.model()->index(i, 0, index0);
        }
        return false;
    }
    return name.contains(m_filter, Qt::CaseInsensitive);
}

void TargetFilterProxyModel::setFilter(const QString &filter)
{
    m_filter = filter;
    invalidateFilter();
}

bool TargetFilterProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QModelIndex srcIndex = mapToSource(index);
    return sourceModel()->setData(srcIndex, value, role);
}
