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
        static const auto branchIcon = QIcon(QStringLiteral(":/kxmlgui5/kateproject/sc-apps-git.svg"));
        return branchIcon;
    } else if (role == Role::CheckoutName) {
        return branch.type == GitUtils::RefType::Remote ? branch.name.mid(branch.remote.size() + 1) : branch.name;
    } else if (role == Role::RefType) {
        return branch.type;
    }

    return {};
}

void BranchesDialogModel::refresh(QVector<GitUtils::Branch> branches)
{
    beginResetModel();
    m_modelEntries = std::move(branches);
    endResetModel();
}
