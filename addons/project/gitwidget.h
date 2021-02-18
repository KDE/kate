/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef GITWIDGET_H
#define GITWIDGET_H

#include <QFutureWatcher>
#include <QProcess>
#include <QWidget>
#include <memory>

#include "git/gitstatus.h"

class QTreeView;
class QPushButton;
class QStringListModel;
class GitStatusModel;
class KateProject;
class QItemSelection;
class QMenu;
class QToolButton;
class QTemporaryFile;

namespace KTextEditor
{
class MainWindow;
class View;
}

class GitWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GitWidget(KateProject *project, QWidget *parent = nullptr, KTextEditor::MainWindow *mainWindow = nullptr);

    bool eventFilter(QObject *o, QEvent *e) override;
    void getStatus(bool untracked = true, bool submodules = false);

private:
    QToolButton *m_menuBtn;
    QPushButton *m_commitBtn;
    QTreeView *m_treeView;
    GitStatusModel *m_model;
    KateProject *m_project;
    /** This ends with "/", always remember this */
    QString m_gitPath;
    QProcess git;
    QFutureWatcher<GitUtils::GitParsedStatus> m_gitStatusWatcher;
    QString m_commitMessage;
    KTextEditor::MainWindow *m_mainWin;
    QMenu *m_gitMenu;
    using TempFileViewPair = std::pair<std::unique_ptr<QTemporaryFile>, KTextEditor::View *>;
    std::vector<TempFileViewPair> m_filesOpenAtHEAD;

    void buildMenu();
    void initGitExe();
    void runGitCmd(const QStringList &args, const char *error);
    void stage(const QStringList &files, bool = false);
    void unstage(const QStringList &files);
    void discard(const QStringList &files);
    void openAtHEAD(const QString &file);
    void showDiff(const QString &file, bool staged);
    void launchExternalDiffTool(const QString &file, bool staged);
    void commitChanges(const QString &msg, const QString &desc);
    void sendMessage(const QString &message, bool warn);

    void hideEmptyTreeNodes();
    void treeViewContextMenuEvent(QContextMenuEvent *e);
    void selectedContextMenu(QContextMenuEvent *e);

    Q_SLOT void gitStatusReady(int exit, QProcess::ExitStatus);
    Q_SLOT void parseStatusReady();
    Q_SLOT void opencommitChangesDialog();

    // signals
public:
    Q_SIGNAL void checkoutBranch();
};

#endif // GITWIDGET_H
