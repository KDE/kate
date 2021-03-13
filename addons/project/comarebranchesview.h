#ifndef COMAREBRANCHESVIEW_H
#define COMAREBRANCHESVIEW_H

#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QWidget>

#include <kateproject.h>

class KateProjectPluginView;
class CompareBranchesView : public QWidget
{
    Q_OBJECT
public:
    explicit CompareBranchesView(QWidget *parent, const QString &gitPath, const QString fromB, const QString &toBr, QStringList files);
    void setPluginView(KateProjectPluginView *pv)
    {
        m_pluginView = pv;
    }

    Q_SIGNAL void backClicked();

private Q_SLOTS:
    void loadFilesDone(const KateProjectSharedQStandardItem &topLevel, KateProjectSharedQHashStringItem file2Item);
    void showDiff(const QModelIndex &idx);

private:
    QPushButton m_backBtn;
    QTreeView m_tree;
    QStringList m_files;
    QStandardItemModel m_model;
    QString m_gitDir;
    QString m_fromBr;
    QString m_toBr;
    KateProjectPluginView *m_pluginView;
};

#endif // COMAREBRANCHESVIEW_H
