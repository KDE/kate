/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QFutureWatcher>
#include <QMenu>

#include "git/gitutils.h"
#include "quickdialog.h"

class QTreeView;
class QLineEdit;
class BranchesDialogModel;
class QAction;
class BranchFilterModel;
class KActionCollection;
class KateProjectPluginView;

namespace KTextEditor
{
class MainWindow;
}

class BranchesDialog : public QuickDialog
{
    Q_OBJECT
public:
    BranchesDialog(QWidget *window, KateProjectPluginView *pluginView, QString projectPath);
    void openDialog();
    // removed, otherwise we can trigger multiple project reloads on branch change
    //    Q_SIGNAL void branchChanged(const QString &branch);

private Q_SLOTS:
    void slotReturnPressed() override;
    void reselectFirst();
    void onCheckoutDone();

private:
    void resetValues();
    void sendMessage(const QString &message, bool warn);
    void createNewBranch(const QString &branch, const QString &fromBranch = QString());

private:
    BranchesDialogModel *m_model;
    BranchFilterModel *m_proxyModel;
    KateProjectPluginView *m_pluginView;
    QString m_projectPath;
    QFutureWatcher<GitUtils::CheckoutResult> m_checkoutWatcher;
    QString m_checkoutBranchName;
    bool m_checkingOutFromBranch = false;
};
