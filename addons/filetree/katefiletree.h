/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QIcon>
#include <QTreeView>
#include <QUrl>

namespace KTextEditor
{
class Document;
class MainWindow;
}

class QActionGroup;

class KateFileTreeModel;
class KateFileTreeProxyModel;

class KateFileTree : public QTreeView
{
    Q_OBJECT

public:
    KateFileTree(KTextEditor::MainWindow *mainWindow, QWidget *parent);
    ~KateFileTree() override;

    void setModel(QAbstractItemModel *model) override;
    void setShowCloseButton(bool show);
    void setMiddleClickToClose(bool value);

public Q_SLOTS:
    void slotDocumentClose();
    void slotExpandRecursive();
    void slotCollapseRecursive();
    void slotDocumentCloseOther();
    void slotDocumentReload();
    void slotOpenContainingFolder();
    void slotCopyFilename();
    void slotCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void slotDocumentFirst();
    void slotDocumentLast();
    void slotDocumentNext();
    void slotDocumentPrev();
    void slotPrintDocument();
    void slotPrintDocumentPreview();
    void slotResetHistory();
    void slotDocumentDelete();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    bool eventFilter(QObject *o, QEvent *e) override;

Q_SIGNALS:
    void viewModeChanged(bool treeMode);
    void sortRoleChanged(int);

private Q_SLOTS:
    void mouseClicked(const QModelIndex &index);

    void slotTreeMode();
    void slotListMode();

    void slotSortName();
    void slotSortPath();
    void slotSortOpeningOrder();
    void slotFixOpenWithMenu(QMenu *menu);
    void slotOpenWithMenuAction(QAction *a);

    void slotRenameFile();

    void onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);

private:
    void closeClicked(const QModelIndex &index);
    using Func = void (KateFileTree::*)();
    QAction *setupOption(QActionGroup *group,
                         const QIcon &icon,
                         const QString &text,
                         const QString &whatisThis,
                         const Func &slot,
                         Qt::CheckState checked = Qt::Unchecked);

    void addChildrenTolist(const QModelIndex &index, QList<QPersistentModelIndex> *worklist);

    KateFileTreeProxyModel *m_proxyModel = nullptr;
    KateFileTreeModel *m_sourceModel = nullptr;
    QPersistentModelIndex m_previouslySelected;
    QPersistentModelIndex m_indexContextMenu;

    bool m_hasCloseButton = false;
    bool m_middleClickToClose = false;

    KTextEditor::MainWindow *m_mainWindow;
};
