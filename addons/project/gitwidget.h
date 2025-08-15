/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QWidget>

#include "git/gitstatus.h"

class QTreeView;
class QStringListModel;
class GitStatusModel;
class KateProject;
class QItemSelection;
class QMenu;
class QToolButton;
class QTemporaryFile;
class KateProjectPluginView;
class GitWidgetTreeView;
class QStackedWidget;
class QLineEdit;
class KActionCollection;
template<typename T>
class QFutureWatcher;

namespace KTextEditor
{
class MainWindow;
class View;
class Document;
}

enum class ClickAction : uint8_t;
enum class StashMode : uint8_t;

class GitWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GitWidget(KTextEditor::MainWindow *mainWindow, KateProjectPluginView *pluginView, QWidget *parent);
    ~GitWidget() override;

    void init();

    bool eventFilter(QObject *o, QEvent *e) override;

    void showEvent(QShowEvent *e) override;

    /**
     * Trigger the GitWidget to update itself.
     * It is safe to call it repeatedly in a short time, due to delayed update after the last call.
     */
    void updateStatus();

    bool isInitialized() const
    {
        return m_initialized;
    }

    KTextEditor::MainWindow *mainWindow();

    // will just proxy the message to the plugin view
    void sendMessage(const QString &message, bool warn);

    QString dotGitPath() const
    {
        return m_activeGitDirPath;
    }

    QString indexPath() const
    {
        return m_gitIndexFilePath;
    }

    // Called from project view when project is changed
    void updateGitProjectFolder();

private:
    // The absolute path to the ".git/index" file
    QString m_gitIndexFilePath;

    /** These ends with "/", always remember this */

    // This variable contains the current active git path
    // being tracked by this widget
    QString m_activeGitDirPath;
    // This variable contains the topLevel git path
    QString m_topLevelGitPath;
    // This variable contains all submodule paths
    QStringList m_submodulePaths;

    /**
     * Helper to avoid multiple reloads at a time
     * @see slotUpdateStatus
     */
    QTimer m_updateTrigger;

    QToolButton *m_menuBtn = nullptr;
    QToolButton *m_commitBtn = nullptr;
    QToolButton *m_pushBtn = nullptr;
    QToolButton *m_pullBtn = nullptr;
    QToolButton *m_cancelBtn = nullptr;
    GitWidgetTreeView *m_treeView = nullptr;
    GitStatusModel *m_model = nullptr;
    QLineEdit *m_filterLineEdit = nullptr;
    QFutureWatcher<GitUtils::GitParsedStatus> *m_gitStatusWatcher;
    QString m_commitMessage;
    KTextEditor::MainWindow *m_mainWin;
    QMenu *m_gitMenu = nullptr;
    KateProjectPluginView *m_pluginView;
    bool m_initialized = false;
    class BusyWrapperWidget *m_refreshButton = nullptr;

    QWidget *m_mainView = nullptr;
    QStackedWidget *m_stackWidget = nullptr;

    using CancelHandle = QPointer<QProcess>;
    CancelHandle m_cancelHandle;

    void setDotGitPath();
    void setSubmodulesPaths();
    void setActiveGitDir();
    void selectActiveFileInStatus();

    QProcess *gitp(const QStringList &arguments);

    void buildMenu(KActionCollection *ac);
    void runGitCmd(const QStringList &args, const QString &i18error);
    void runPushPullCmd(const QStringList &args);
    void stage(const QStringList &files, bool = false);
    void unstage(const QStringList &files);
    void discard(const QStringList &files);
    void clean(const QStringList &files);
    void openAtHEAD(const QString &file);
    void showDiff(const QString &file, bool staged);
    void launchExternalDiffTool(const QString &file, bool staged);
    void commitChanges(const QString &msg, const QString &desc, bool signOff, bool amend = false);
    void branchCompareFiles(const QString &from, const QString &to);

    QMenu *stashMenu(KActionCollection *pCollection);
    QAction *stashMenuAction(KActionCollection *ac, const QString &name, const QString &text, StashMode m);

    void treeViewContextMenuEvent(QContextMenuEvent *e);
    void selectedContextMenu(QContextMenuEvent *e);

    void createStashDialog(StashMode m, const QString &gitPath);

    void enableCancel(QProcess *git);
    void hideCancel();

private:
    /**
     * Does the real update
     */
    void slotUpdateStatus();

    void parseStatusReady();
    void openCommitChangesDialog(bool amend = false);
    void handleClick(const QModelIndex &idx, ClickAction clickAction);
    void treeViewSingleClicked(const QModelIndex &idx);
    void treeViewDoubleClicked(const QModelIndex &idx);

Q_SIGNALS:
    void statusUpdated(const GitUtils::GitParsedStatus &status);
};
