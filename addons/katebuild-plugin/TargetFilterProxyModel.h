/***************************************************************************
 *   This file is part of Kate build plugin                                *
 *   SPDX-FileCopyrightText: 2014 Kåre Särs <kare.sars@iki.fi>                           *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 ***************************************************************************/
#ifndef TargetFilterProxyModel_H
#define TargetFilterProxyModel_H

#include <QModelIndex>
#include <QObject>
#include <QSortFilterProxyModel>
#include <QString>

class TargetFilterProxyModel : public QSortFilterProxyModel
{
public:
    TargetFilterProxyModel(QObject *parent = nullptr);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    void setFilter(const QString &filter);
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QString m_filter;
};

#endif
