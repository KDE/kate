#ifndef GIT_BLAME_FILE_TREE_VIEW
#define GIT_BLAME_FILE_TREE_VIEW

#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QWidget>

struct GitFileItem {
    QByteArray file;
    int linesAdded;
    int linesRemoved;
};

/**
 * This class provides a way to show the diff tree for
 * a commit.
 *
 * For now this is only being used by the git
 * blame plugin, but later it can be reused by *any* plugin
 * which wants to show the tree for a commit. One such user
 * could be the "File History" viewer in kate project plugin.
 * Hence this class is written in a way to not depend on any plugin
 * specific stuff.
 */
class CommitDiffTreeView : public QWidget
{
    Q_OBJECT
public:
    explicit CommitDiffTreeView(QWidget *parent);

    /**
     * open treeview for commit with @p hash
     * @filePath can be path of any file in the repo
     */
    void openCommit(const QString &hash, const QString &filePath);

    Q_SIGNAL void closeRequested();
    Q_SIGNAL void showDiffRequested(const QByteArray &diffContents);

public:
    void createFileTreeForCommit(const QString &filePath, const QByteArray &rawNumStat);

private Q_SLOTS:
    void showDiff(const QModelIndex &idx);

private:
    QPushButton m_backBtn;
    QTreeView m_tree;
    QStandardItemModel m_model;
    QString m_gitDir;
    QString m_commitHash;
};

#endif
