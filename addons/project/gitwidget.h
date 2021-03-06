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
class QStringListModel;
class GitStatusModel;
class KateProject;
class QItemSelection;
class QMenu;
class QToolButton;
class QTemporaryFile;
class KateProjectPluginView;
class GitWidgetTreeView;

namespace KTextEditor
{
class MainWindow;
class View;
class Document;
}

enum class ClickAction : uint8_t;

class GitWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GitWidget(KateProject *project, KTextEditor::MainWindow *mainWindow = nullptr, KateProjectPluginView *pluginView = nullptr);

    bool eventFilter(QObject *o, QEvent *e) override;
    void getStatus(bool untracked = true, bool submodules = false);
    QProcess *gitprocess();
    KTextEditor::MainWindow *mainWindow();

    using TempFileViewPair = std::pair<std::unique_ptr<QTemporaryFile>, QPointer<KTextEditor::View>>;
    std::vector<TempFileViewPair> *tempFilesVector();
    void openTempFile(const QString &file, const QString &templatString, const QByteArray &contents);

    // will just proxy the message to the plugin view
    void sendMessage(const QString &message, bool warn);

private:
    QToolButton *m_menuBtn;
    QToolButton *m_commitBtn;
    QToolButton *m_pushBtn;
    QToolButton *m_pullBtn;
    GitWidgetTreeView *m_treeView;
    GitStatusModel *m_model;
    KateProject *m_project;
    /** This ends with "/", always remember this */
    QString m_gitPath;
    QProcess git;
    QFutureWatcher<GitUtils::GitParsedStatus> m_gitStatusWatcher;
    QString m_commitMessage;
    KTextEditor::MainWindow *m_mainWin;
    QMenu *m_gitMenu;
    std::vector<TempFileViewPair> m_tempFiles;
    KateProjectPluginView *m_pluginView;

    void buildMenu();
    void initGitExe();
    void runGitCmd(const QStringList &args, const QString &i18error);
    void runPushPullCmd(const QStringList &args);
    void stage(const QStringList &files, bool = false);
    void unstage(const QStringList &files);
    void discard(const QStringList &files);
    void clean(const QStringList &files);
    void openAtHEAD(const QString &file);
    void showDiff(const QString &file, bool staged);
    void launchExternalDiffTool(const QString &file, bool staged);
    void commitChanges(const QString &msg, const QString &desc, bool signOff);
    void applyDiff(const QString &fileName, bool staged, bool hunk, KTextEditor::View *v);
    void numStatForStatus(QVector<GitUtils::StatusItem> &list, bool modified);
    QMenu *stashMenu();

    void hideEmptyTreeNodes();
    void treeViewContextMenuEvent(QContextMenuEvent *e);
    void selectedContextMenu(QContextMenuEvent *e);

    QString getDiff(KTextEditor::View *view, bool hunk, bool alreadyStaged);

public Q_SLOTS:
    void clearTempFile(KTextEditor::Document *document);

private Q_SLOTS:
    void gitStatusReady(int exit, QProcess::ExitStatus);
    void parseStatusReady();
    void opencommitChangesDialog();
    void handleClick(const QModelIndex &idx, ClickAction clickAction);
    void treeViewSingleClicked(const QModelIndex &idx);
    void treeViewDoubleClicked(const QModelIndex &idx);

    // signals
public:
    Q_SIGNAL void checkoutBranch();
};

#endif // GITWIDGET_H
