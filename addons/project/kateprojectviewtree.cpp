/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectviewtree.h"
#include "kateprojectfiltermodel.h"
#include "kateprojectpluginview.h"
#include "kateprojecttreeviewcontextmenu.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QContextMenuEvent>

KateProjectViewTree::KateProjectViewTree(KateProjectPluginView *pluginView, KateProject *project)
    : m_pluginView(pluginView)
    , m_project(project)
{
    /**
     * default style
     */
    setHeaderHidden(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAllColumnsShowFocus(true);
    setFocusPolicy(Qt::NoFocus);

    /**
     * attach view => project
     * do this once, model is stable for whole project life time
     * kill selection model
     * create sort proxy model
     */
    QItemSelectionModel *m = selectionModel();

    KateProjectFilterProxyModel *sortModel = new KateProjectFilterProxyModel(this);

    // sortModel->setFilterRole(SortFilterRole);
    // sortModel->setSortRole(SortFilterRole);
    sortModel->setRecursiveFilteringEnabled(true);
    sortModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    sortModel->setSourceModel(m_project->model());
    setModel(sortModel);
    delete m;

    /**
     * connect needed signals
     * we use activated + clicked as we want "always" single click activation + keyboard focus / enter working
     */
    connect(this, &KateProjectViewTree::activated, this, &KateProjectViewTree::slotClicked);
    connect(this, &KateProjectViewTree::clicked, this, &KateProjectViewTree::slotClicked);
    connect(m_project, &KateProject::modelChanged, this, &KateProjectViewTree::slotModelChanged);

    /**
     * trigger once some slots
     */
    slotModelChanged();
}

KateProjectViewTree::~KateProjectViewTree()
{
}

void KateProjectViewTree::selectFile(const QString &file)
{
    /**
     * get item if any
     */
    QStandardItem *item = m_project->itemForFile(file);
    if (!item) {
        return;
    }

    /**
     * select it
     */
    QModelIndex index = static_cast<QSortFilterProxyModel *>(model())->mapFromSource(m_project->model()->indexFromItem(item));
    scrollTo(index, QAbstractItemView::EnsureVisible);
    selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear | QItemSelectionModel::Select);
}

void KateProjectViewTree::openSelectedDocument()
{
    /**
     * anything selected?
     */
    QModelIndexList selecteStuff = selectedIndexes();
    if (selecteStuff.isEmpty()) {
        return;
    }

    /**
     * we only handle files here!
     */
    if (selecteStuff[0].data(KateProjectItem::TypeRole).toInt() != KateProjectItem::File) {
        return;
    }

    /**
     * open document for first element, if possible
     */
    QString filePath = selecteStuff[0].data(Qt::UserRole).toString();
    if (!filePath.isEmpty()) {
        m_pluginView->mainWindow()->openUrl(QUrl::fromLocalFile(filePath));
    }
}

void KateProjectViewTree::slotClicked(const QModelIndex &index)
{
    /**
     * open document, if any usable user data
     */
    const QString filePath = index.data(Qt::UserRole).toString();
    if (!filePath.isEmpty()) {
        /**
         * normal file? => just trigger open of it
         */
        if (index.data(KateProjectItem::TypeRole).toInt() == KateProjectItem::File) {
            m_pluginView->mainWindow()->openUrl(QUrl::fromLocalFile(filePath));
            selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear | QItemSelectionModel::Select);
            return;
        }

        /**
         * linked project? => switch the current active project
         */
        if (index.data(KateProjectItem::TypeRole).toInt() == KateProjectItem::LinkedProject) {
            m_pluginView->switchToProject(QDir(filePath));
            return;
        }
    }
}

void KateProjectViewTree::slotModelChanged()
{
    /**
     * model was updated
     * perhaps we need to highlight again new file
     */
    KTextEditor::View *activeView = m_pluginView->mainWindow()->activeView();
    if (activeView && activeView->document()->url().isLocalFile()) {
        selectFile(activeView->document()->url().toLocalFile());
    }
}

void KateProjectViewTree::contextMenuEvent(QContextMenuEvent *event)
{
    /**
     * get path file path or don't do anything
     */
    QModelIndex index = selectionModel()->currentIndex();
    QString filePath = index.data(Qt::UserRole).toString();
    if (filePath.isEmpty()) {
        QTreeView::contextMenuEvent(event);
        return;
    }

    KateProjectTreeViewContextMenu menu;
    menu.exec(filePath, index, viewport()->mapToGlobal(event->pos()), this);

    event->accept();
}
