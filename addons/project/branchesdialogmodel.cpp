/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "branchesdialogmodel.h"

#include <QFont>
#include <QIcon>

#include <KLocalizedString>

static BranchesDialogModel::ItemType itemType(const GitUtils::Branch &branch)
{
    if (branch.refType != GitUtils::RefType::None) {
        return BranchesDialogModel::BranchItem;
    }
    return branch.name == BranchesDialogModel::createBranchItemName() ? BranchesDialogModel::CreateBranch : BranchesDialogModel::CreateBranchFrom;
}

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
    if (role == Qt::DisplayRole) {
        return branch.name;
    } else if (role == Role::FuzzyScore) {
        return branch.score;
    } else if (role == Qt::DecorationRole) {
        if (itemType(branch) == BranchItem) {
            static const auto branchIcon = QIcon::fromTheme(QStringLiteral("vcs-branch"));
            return branchIcon;
        } else {
            static const auto addIcon = QIcon::fromTheme(QStringLiteral("list-add"));
            return addIcon;
        }
    } else if (role == Qt::FontRole) {
        if (itemType(branch) != BranchItem) {
            QFont font;
            font.setBold(true);
            return font;
        }
    } else if (role == Role::CheckoutName) {
        return branch.refType == GitUtils::RefType::Remote ? branch.name.mid(branch.remote.size() + 1) : branch.name;
    } else if (role == Role::RefType) {
        return branch.refType;
    } else if (role == Role::ItemTypeRole) {
        return itemType(branch);
    }

    return {};
}

void BranchesDialogModel::refresh(QList<GitUtils::Branch> branches)
{
    beginResetModel();
    m_modelEntries = std::move(branches);
    endResetModel();
}

void BranchesDialogModel::clear()
{
    beginResetModel();
    m_modelEntries.clear();
    endResetModel();
}

void BranchesDialogModel::clearBranchCreationItems()
{
    beginRemoveRows(QModelIndex(), 0, 1);
    m_modelEntries.removeFirst();
    m_modelEntries.removeFirst();
    endRemoveRows();
}

QString BranchesDialogModel::createBranchItemName()
{
    static const auto s = i18n("Create New Branch");
    return s;
}

QString BranchesDialogModel::createFromBranchItemName()
{
    static const auto s = i18n("Create New Branch From...");
    return s;
}
