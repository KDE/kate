/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATEQUICKOPENMODEL_H
#define KATEQUICKOPENMODEL_H

#include <QAbstractTableModel>
#include <QVariant>
#include <QVector>

#include "gitutils.h"

class BranchesDialogModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Role {
        Score = Qt::UserRole + 1,
        DisplayName,
        CheckoutName,
        RefType,
    };
    explicit BranchesDialogModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &idx, int role) const override;
    void refresh(QVector<GitUtils::Branch> branches);

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (!index.isValid()) {
            return false;
        }
        if (role == Role::Score) {
            auto row = index.row();
            m_modelEntries[row].score = value.toInt();
        }
        return QAbstractTableModel::setData(index, value, role);
    }

private:
    QVector<GitUtils::Branch> m_modelEntries;
};

#endif
