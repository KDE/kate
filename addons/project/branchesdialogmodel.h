/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QVariant>

#include "git/gitutils.h"

class BranchesDialogModel : public QAbstractTableModel
{
public:
    enum Role {
        FuzzyScore = Qt::UserRole + 1,
        CheckoutName,
        RefType,
        Creator,
        ItemTypeRole,
        LastActivityRole,
    };
    enum ItemType {
        BranchItem,
        CreateBranch,
        CreateBranchFrom
    };

    explicit BranchesDialogModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &idx, int role) const override;
    void refresh(QList<GitUtils::Branch> branches);
    void clear();
    void clearBranchCreationItems();

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (!index.isValid()) {
            return false;
        }
        if (role == Role::FuzzyScore) {
            auto row = index.row();
            m_modelEntries[row].score = value.toInt();
        }
        return QAbstractTableModel::setData(index, value, role);
    }

    static QString createBranchItemName();
    static QString createFromBranchItemName();

private:
    QList<GitUtils::Branch> m_modelEntries;
};
