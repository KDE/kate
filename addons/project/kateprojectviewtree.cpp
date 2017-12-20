/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kateprojectviewtree.h"
#include "kateprojectpluginview.h"
#include "kateprojecttreeviewcontextmenu.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QContextMenuEvent>
#include <krecursivefilterproxymodel.h>

KateProjectViewTree::KateProjectViewTree(KateProjectPluginView *pluginView, KateProject *project)
    : QTreeView()
    , m_pluginView(pluginView)
    , m_project(project)
{
    /**
     * default style
     */
    setHeaderHidden(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    /**
     * attach view => project
     * do this once, model is stable for whole project life time
     * kill selection model
     * create sort proxy model
     */
    QItemSelectionModel *m = selectionModel();
    QSortFilterProxyModel *sortModel = new KRecursiveFilterProxyModel(this);
    //sortModel->setFilterRole(SortFilterRole);
    //sortModel->setSortRole(SortFilterRole);
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
    QString filePath = index.data(Qt::UserRole).toString();
    if (!filePath.isEmpty()) {
        m_pluginView->mainWindow()->openUrl(QUrl::fromLocalFile(filePath));
        selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear | QItemSelectionModel::Select);
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
    menu.exec(filePath, viewport()->mapToGlobal(event->pos()), this);

    event->accept();
}
