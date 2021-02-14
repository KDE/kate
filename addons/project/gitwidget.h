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

class GitWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GitWidget(KateProject *project, QWidget *parent = nullptr);

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

    void getStatus(const QString &repo, bool untracked = true, bool submodules = false);
    void stageAll(bool untracked = false);

    GitParsedStatus parseStatus(const QByteArray &raw);
    void hideEmptyTreeNodes();
    void treeViewContextMenuEvent(QContextMenuEvent *e);

    Q_SLOT void gitStatusReady();
    Q_SLOT void parseStatusReady();
};

#endif // GITWIDGET_H
