/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef GITWIDGET_H
#define GITWIDGET_H

#include <QFutureWatcher>
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
    explicit GitWidget(KateProject *project, KTextEditor::MainWindow *mainWindow = nullptr, KateProjectPluginView *pluginView = nullptr);
    ~GitWidget() override;

    void init();

    bool eventFilter(QObject *o, QEvent *e) override;

    void showEvent(QShowEvent *e) override;

    /**
     * Trigger the GitWidget to update itself.
     * It is safe to call it repeatedly in a short time, due to delayed update after the last call.
     */
    void updateStatus();

    KTextEditor::MainWindow *mainWindow();

    // will just proxy the message to the plugin view
    void sendMessage(const QString &message, bool warn);

    QString dotGitPath() const
    {
        return m_activeGitDirPath;
    }

    QString indexPath() const
    {
        if (m_activeGitDirPath == m_topLevelGitPath) {
            return m_activeGitDirPath + QStringLiteral(".git/index");
        }
        // Should we support submodules?
        return QString();
    }

private:
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

    QToolButton *m_menuBtn;
    QToolButton *m_commitBtn;
    QToolButton *m_pushBtn;
    QToolButton *m_pullBtn;
    QToolButton *m_cancelBtn;
    KateProject *m_project;
    GitWidgetTreeView *m_treeView;
    GitStatusModel *m_model;
    QLineEdit *m_filterLineEdit;
    QFutureWatcher<GitUtils::GitParsedStatus> m_gitStatusWatcher;
    QString m_commitMessage;
    KTextEditor::MainWindow *m_mainWin;
    QMenu *m_gitMenu;
    KateProjectPluginView *m_pluginView;
    bool m_initialized = false;

    QWidget *m_mainView;
    QStackedWidget *m_stackWidget;

    using CancelHandle = QPointer<QProcess>;
    CancelHandle m_cancelHandle;

    void setDotGitPath();
    void setSubmodulesPaths();
    void setActiveGitDir();

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

private Q_SLOTS:
    /**
     * Does the real update
     */
    void slotUpdateStatus();

    void parseStatusReady();
    void openCommitChangesDialog(bool amend = false);
    void handleClick(const QModelIndex &idx, ClickAction clickAction);
    void treeViewSingleClicked(const QModelIndex &idx);
    void treeViewDoubleClicked(const QModelIndex &idx);

    // signals
public:
    Q_SIGNAL void checkoutBranch();
};

#endif // GITWIDGET_H
