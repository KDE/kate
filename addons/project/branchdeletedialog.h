/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDialog>
#include <QStandardItemModel>
#include <QTreeView>

class BranchDeleteDialog : public QDialog
{
public:
    explicit BranchDeleteDialog(const QString &dotGitPath, QWidget *parent = nullptr);
    QStringList branchesToDelete() const;

private:
    void loadBranches(const QString &dotGitPath);
    void updateLabel(QStandardItem *item);
    void onCheckAllClicked(bool);
    QStandardItemModel m_model;
    QTreeView m_listView;
};
