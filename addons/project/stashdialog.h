/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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

enum class StashMode : uint8_t {
    None = 0,
    Stash,
    StashKeepIndex,
    StashUntrackIncluded,
    StashPopLast,
    StashPop,
    StashDrop,
    StashApply,
    StashApplyLast,
    ShowStashContent,
    ShowStashEntry
};

class StashDialog : public HUDDialog
{
    Q_OBJECT
public:
    StashDialog(QWidget *window, GitWidget *gitWidget, const QString &gitPath);

    void openDialog(StashMode mode);

protected Q_SLOTS:
    void slotReturnPressed(const QModelIndex &index) override;

private:
    QProcess *gitp(const QStringList &arguments);
    void stash(bool keepIndex, bool includeUntracked);
    void getStashList();
    void popStash(const QString &index, const QString &command = QStringLiteral("pop"));
    void applyStash(const QString &index);
    void dropStash(const QString &index);
    void showStash(const QString &index);

    // the git processes report to the git widget, the dialog might be gone by the time they finish
    GitWidget *const m_gitWidget;
    QString m_gitPath;
    QString m_projectPath;
    StashMode m_currentMode = StashMode::None;
    // a stash operation is running, e.g. don't drop a second stash on a second Enter
    bool m_busy = false;
};
