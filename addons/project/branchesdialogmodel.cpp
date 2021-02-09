/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "branchesdialogmodel.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <QIcon>

BranchesDialogModel::BranchesDialogModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int BranchesDialogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_modelEntries.size();
}

int BranchesDialogModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant BranchesDialogModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid()) {
        return {};
    }

    const Branch &branch = m_modelEntries.at(idx.row());
    if (role == Qt::DisplayRole) {
        return branch.name;
    } else if (role == Role::FuzzyScore) {
        return branch.score;
    } else if (role == Role::OriginalSorting) {
        return branch.dateSort;
    } else if (role == Qt::DecorationRole) {
        static const auto branchIcon = QIcon(QStringLiteral(":/kxmlgui5/kateproject/sc-apps-git.svg"));
        return branchIcon;
    } else if (role == Role::CheckoutName) {
        return branch.refType == GitUtils::RefType::Remote ? branch.name.mid(branch.remote.size() + 1) : branch.name;
    } else if (role == Role::RefType) {
        return branch.refType;
    } else if (role == Role::ItemTypeRole) {
        return branch.itemType;
    }

    return {};
}

void BranchesDialogModel::refresh(QVector<GitUtils::Branch> branches)
{
    // clear
    Branch create{branches.at(0).name, {}, {}, 0, 0, ItemType::CreateBranch};
    Branch createFrom{branches.at(1).name, {}, {}, 0, 1, ItemType::CreateBranchFrom};

    QVector<Branch> temp{create, createFrom};

    for (int i = 2; i < branches.size(); ++i) {
        temp.append({branches.at(i).name, branches.at(i).remote, branches.at(i).type, -1, i, ItemType::BranchItem});
    }

    beginResetModel();
    m_modelEntries = std::move(temp);
    endResetModel();
}

void BranchesDialogModel::clear()
{
    beginResetModel();
    QVector<Branch>().swap(m_modelEntries);
    endResetModel();
}

void BranchesDialogModel::clearBranchCreationItems()
{
    beginRemoveRows(QModelIndex(), 0, 1);
    m_modelEntries.removeFirst();
    m_modelEntries.removeFirst();
    endRemoveRows();
}
