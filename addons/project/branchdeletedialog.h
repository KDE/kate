/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <QDialog>
#include <QStandardItemModel>
#include <QTreeView>

class BranchDeleteDialog : public QDialog
{
    Q_OBJECT
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
