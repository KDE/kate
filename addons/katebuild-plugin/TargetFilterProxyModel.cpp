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
    QModelIndex srcIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!srcIndex.isValid()) {
        qDebug() << "srcIndex is invalid";
        return false;
    }

    if (m_filter.isEmpty()) {
        return true;
    }

    QString name = srcIndex.data().toString();
    if (name.contains(m_filter, Qt::CaseInsensitive)) {
        return true;
    }

    for (int row = 0; row < sourceModel()->rowCount(srcIndex); ++row) {
        const QModelIndex childIndex = srcIndex.model()->index(row, 0, srcIndex);
        name = childIndex.data().toString();
        if (name.contains(m_filter, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
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
