/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QFutureWatcher>
#include <QMenu>

#include "git/gitutils.h"

class QTreeView;
class QLineEdit;
class BranchesDialogModel;
class QAction;
class BranchFilterModel;
class KActionCollection;

namespace KTextEditor
{
class MainWindow;
}

class BranchesDialog : public QMenu
{
    Q_OBJECT
public:
    BranchesDialog(QWidget *parent, KTextEditor::MainWindow *mainWindow, QString projectPath);

    void openDialog();

    void updateViewGeometry();

    Q_SIGNAL void branchChanged(const QString &branch);


protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void slotReturnPressed();
    void reselectFirst();
    void onCheckoutDone();

private:
    void resetValues();
    void sendMessage(const QString &message, bool warn);
    void createNewBranch(const QString &branch, const QString &fromBranch = QString());

private:
    QTreeView *m_treeView;
    QLineEdit *m_lineEdit;
    BranchesDialogModel *m_model;
    BranchFilterModel *m_proxyModel;
    KTextEditor::MainWindow *m_mainWindow;
    QString m_projectPath;
    QFutureWatcher<GitUtils::CheckoutResult> m_checkoutWatcher;
    QString m_checkoutBranchName;
    bool m_checkingOutFromBranch = false;
};
