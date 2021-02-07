/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2018 Tomaz Canabrava <tcanabrava@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "branchesdialogmodel.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

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

    const GitUtils::Branch &branch = m_modelEntries.at(idx.row());
    if (role == Role::DisplayName) {
        return branch.name;
    } else if (role == Role::Score) {
        return branch.score;
    } else if (role == Qt::DecorationRole) {
        static const auto branchIcon = QIcon(QStringLiteral(":/kxmlgui5/kateproject/sc-apps-git.svg"));
        return branchIcon;
    } else if (role == Role::Commit) {
        return branch.commit.mid(0, 7);
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
