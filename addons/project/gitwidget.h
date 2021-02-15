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

private:
    struct GitParsedStatus {
        QVector<GitUtils::StatusItem> untracked;
        QVector<GitUtils::StatusItem> unmerge;
        QVector<GitUtils::StatusItem> staged;
        QVector<GitUtils::StatusItem> changed;
    };

    QPushButton *m_menuBtn;
    QPushButton *m_commitBtn;
    QTreeView *m_treeView;
    GitStatusModel *m_model;
    KateProject *m_project;
    QProcess git;
    QFutureWatcher<GitParsedStatus> m_gitStatusWatcher;
    QString m_commitMessage;
    KTextEditor::MainWindow *m_mainWin;

    void getStatus(const QString &repo, bool untracked = true, bool submodules = false);
    void stage(const QStringList &files, bool untracked = false);
    void unstage(const QStringList &files);
    void commitChanges(const QString &msg, const QString &desc);
    void sendMessage(const QString &message, bool warn);

    GitParsedStatus parseStatus(const QByteArray &raw);
    void hideEmptyTreeNodes();
    void treeViewContextMenuEvent(QContextMenuEvent *e);
    void selectedContextMenu(QContextMenuEvent *e);

    Q_SLOT void gitStatusReady(int exit, QProcess::ExitStatus);
    Q_SLOT void parseStatusReady();
    Q_SLOT void opencommitChangesDialog();
};

#endif // GITWIDGET_H
