/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "branchcheckoutdialog.h"
#include "branchesdialogmodel.h"

#include <KLocalizedString>
#include <QtConcurrentRun>

BranchCheckoutDialog::BranchCheckoutDialog(QWidget *mainWindow, KateProjectPluginView *pluginView, QString projectPath)
    : BranchesDialog(mainWindow, pluginView, projectPath)
{
    connect(&m_checkoutWatcher, &QFutureWatcher<GitUtils::CheckoutResult>::finished, this, &BranchCheckoutDialog::onCheckoutDone);
}

BranchCheckoutDialog::~BranchCheckoutDialog()
{
    if (m_checkoutWatcher.isRunning()) {
        onCheckoutDone();
    }
}

void BranchCheckoutDialog::resetValues()
{
    m_checkoutFromBranchName.clear();
    m_checkingOutFromBranch = false;
    m_lineEdit.setPlaceholderText(i18n("Select branch to checkout. Press 'Esc' to cancel."));
}

void BranchCheckoutDialog::openDialog()
{
    resetValues();
    GitUtils::Branch newBranch;
    newBranch.name = i18n("Create New Branch");
    GitUtils::Branch newBranchFrom;
    newBranchFrom.name = i18n("Create New Branch From...");
    QVector<GitUtils::Branch> branches{newBranch, newBranchFrom};
    branches << GitUtils::getAllBranches(m_projectPath);
    m_model->refresh(branches, /*checkingOut:*/ true);

    reselectFirst();
    updateViewGeometry();
    setFocus();
    exec();
}

void BranchCheckoutDialog::onCheckoutDone()
{
    const GitUtils::CheckoutResult res = m_checkoutWatcher.result();
    bool warn = false;
    QString msgStr = i18n("Branch %1 checked out", res.branch);
    if (res.returnCode > 0) {
        warn = true;
        msgStr = i18n("Failed to checkout to branch %1, Error: %2", res.branch, res.error);
    }

    sendMessage(msgStr, warn);
}

void BranchCheckoutDialog::slotReturnPressed(const QModelIndex &index)
{
    // we cleared the model to checkout new branch
    if (m_model->rowCount() == 0) {
        createNewBranch(m_lineEdit.text(), m_checkoutFromBranchName);
        return;
    }

    if (!index.isValid()) {
        clearLineEdit();
        hide();
        return;
    }

    // branch is selected, do actual checkout
    if (m_checkingOutFromBranch) {
        m_checkingOutFromBranch = false;
        const auto fromBranch = index.data(BranchesDialogModel::CheckoutName).toString();
        m_checkoutFromBranchName = fromBranch;
        m_model->clear();
        clearLineEdit();
        m_lineEdit.setPlaceholderText(i18n("Enter new branch name. Press 'Esc' to cancel."));
        return;
    }

    const auto branch = index.data(BranchesDialogModel::CheckoutName).toString();
    const auto itemType = (BranchesDialogModel::ItemType)index.data(BranchesDialogModel::ItemTypeRole).toInt();

    if (itemType == BranchesDialogModel::BranchItem) {
        QFuture<GitUtils::CheckoutResult> future = QtConcurrent::run(&GitUtils::checkoutBranch, m_projectPath, branch);
        m_checkoutWatcher.setFuture(future);
    } else if (itemType == BranchesDialogModel::CreateBranch) {
        m_model->clear();
        m_lineEdit.setPlaceholderText(i18n("Enter new branch name. Press 'Esc' to cancel."));
        return;
    } else if (itemType == BranchesDialogModel::CreateBranchFrom) {
        m_model->clearBranchCreationItems();
        clearLineEdit();
        m_lineEdit.setPlaceholderText(i18n("Select branch to checkout from. Press 'Esc' to cancel."));
        m_checkingOutFromBranch = true;
        return;
    }

    clearLineEdit();
    hide();
}

void BranchCheckoutDialog::createNewBranch(const QString &branch, const QString &fromBranch)
{
    if (branch.isEmpty()) {
        clearLineEdit();
        hide();
        return;
    }

    // the branch name might be invalid, let git handle it
    const GitUtils::CheckoutResult r = GitUtils::checkoutNewBranch(m_projectPath, branch, fromBranch);
    const bool warn = true;
    if (r.returnCode == 0) {
        sendMessage(i18n("Checked out to new branch: %1", r.branch), !warn);
    } else {
        sendMessage(i18n("Failed to create new branch. Error \"%1\"", r.error), warn);
    }

    clearLineEdit();
    hide();
}
