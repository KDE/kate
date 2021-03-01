/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QMenu>

#include "quickdialog.h"

class QTreeView;
class QLineEdit;
class BranchesDialogModel;
class QAction;
class StashFilterModel;
class KActionCollection;
class QStandardItemModel;
class QProcess;
class GitWidget;

namespace KTextEditor
{
class MainWindow;
}

namespace GitUtils
{
struct CheckoutResult;
}

class StashDialog : public QuickDialog
{
    Q_OBJECT
public:
    enum Mode {
        None,
        Stash,
        StashKeepIndex,
        StashUntrackIncluded,
        StashPopLast,
        StashPop,
        StashDrop,
        StashApply,
        StashApplyLast,
        ShowStashContent,
    };

    StashDialog(GitWidget *gitwidget, QWidget *window);

    void openDialog(Mode mode);

    Q_SIGNAL void branchChanged(const QString &branch);

protected Q_SLOTS:
    void slotReturnPressed() override;

private Q_SLOTS:
    void sendMessage(const QString &message, bool warn);

private:
    void stash(bool keepIndex, bool includeUntracked);
    void getStashList();
    void popStash(const QByteArray &index, const QString &command = QStringLiteral("pop"));
    void applyStash(const QByteArray &index);
    void dropStash(const QByteArray &index);
    void showStash(const QByteArray &index);

    QStandardItemModel *m_model;
    StashFilterModel *m_proxyModel;
    GitWidget *m_gitwidget;
    QString m_projectPath;
    Mode m_currentMode = None;
};
