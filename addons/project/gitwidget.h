#ifndef GITWIDGET_H
#define GITWIDGET_H

#include <QFutureWatcher>
#include <QProcess>
#include <QWidget>

#include "git/gitstatus.h"

class QTreeView;
class QPushButton;
class QStringListModel;
class GitStatusModel;
class KateProject;
class QItemSelection;
class QMenu;
class QToolButton;

namespace KTextEditor
{
class MainWindow;
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
    QProcess git;
    QFutureWatcher<GitUtils::GitParsedStatus> m_gitStatusWatcher;
    QString m_commitMessage;
    KTextEditor::MainWindow *m_mainWin;
    QMenu *m_gitMenu;

    void buildMenu();
    void stage(const QStringList &files, bool = false);
    void unstage(const QStringList &files);
    void discard(const QStringList &files);
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
