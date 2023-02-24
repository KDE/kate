/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QFutureWatcher>
#include <QTimer>
#include <QToolButton>
#include <QUrl>

namespace KTextEditor
{
class MainWindow;
class View;
}

/**
 * @brief a pushbutton that shows the active git branch of the "active view"
 */
class CurrentGitBranchButton : public QToolButton
{
    Q_OBJECT
public:
    explicit CurrentGitBranchButton(KTextEditor::MainWindow *mainWindow, QWidget *parent = nullptr);
    ~CurrentGitBranchButton() override;

    enum BranchType { Branch = 0, Commit, Tag };
    struct BranchResult {
        QString branch;
        BranchType type;
    };

    void refresh();

private:
    void onViewChanged(KTextEditor::View *v);
    void hideButton();
    void onBranchFetched();

    QUrl m_activeUrl;
    QFutureWatcher<BranchResult> m_watcher;
    QTimer m_viewChangedTimer;
};
