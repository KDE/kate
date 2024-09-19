/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectviewtree.h"
#include "kateproject.h"
#include "kateprojectfiltermodel.h"
#include "kateprojectitem.h"
#include "kateprojectpluginview.h"
#include "kateprojecttreeviewcontextmenu.h"
#include "ktexteditor_utils.h"

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QContextMenuEvent>
#include <QDir>
#include <QScrollBar>

#include <KLocalizedString>

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
    setIndentation(12);

    setDragDropMode(QAbstractItemView::DropOnly);
    setDragDropOverwriteMode(false);

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

    connect(this, &QTreeView::expanded, this, [this](const QModelIndex &index) {
        QString path = index.data(Qt::UserRole).toString().remove(m_project->baseDir());
        m_expandedNodes.insert(path);
    });

    connect(this, &QTreeView::collapsed, this, [this](const QModelIndex &index) {
        QString path = index.data(Qt::UserRole).toString().remove(m_project->baseDir());
        m_expandedNodes.remove(path);
    });
    connect(m_project, &KateProject::projectMapChanged, this, [this] {
        m_verticalScrollPosition = verticalScrollBar()->value();
    });

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

void KateProjectViewTree::addFile(const QModelIndex &idx, const QString &fileName)
{
    QStandardItem *item = [idx, this] {
        if (idx.isValid()) {
            auto proxyModel = static_cast<QSortFilterProxyModel *>(model());
            auto index = proxyModel->mapToSource(idx);
            return m_project->model()->itemFromIndex(index);
        }
        return m_project->model()->invisibleRootItem();
    }();
    if (!item) {
        return;
    }

    const QString base = idx.isValid() ? idx.data(Qt::UserRole).toString() : m_project->baseDir();
    const QString fullFileName = base + QLatin1Char('/') + fileName;

    /**
     * Create an actual file on disk
     */
    QFile f(fullFileName);
    bool created = f.open(QIODevice::WriteOnly);
    if (!created) {
        const auto icon = QIcon::fromTheme(QStringLiteral("document-new"));
        Utils::showMessage(i18n("Failed to create file: %1, Error: %2", fileName, f.errorString()), icon, i18n("Project"), MessageType::Error);
        return;
    }

    KateProjectItem *i = new KateProjectItem(KateProjectItem::File, fileName, fullFileName);
    item->appendRow(i);
    m_project->addFile(fullFileName, i);
    item->sortChildren(0);
}

void KateProjectViewTree::addDirectory(const QModelIndex &idx, const QString &name)
{
    QStandardItem *item = [idx, this] {
        if (idx.isValid()) {
            auto proxyModel = static_cast<QSortFilterProxyModel *>(model());
            auto index = proxyModel->mapToSource(idx);
            return m_project->model()->itemFromIndex(index);
        }
        return m_project->model()->invisibleRootItem();
    }();
    if (!item) {
        return;
    }

    const QString base = idx.isValid() ? idx.data(Qt::UserRole).toString() : m_project->baseDir();
    const QString fullDirName = base + QLatin1Char('/') + name;

    QDir dir(base);
    if (!dir.mkdir(name)) {
        const auto icon = QIcon::fromTheme(QStringLiteral("folder-new"));
        Utils::showMessage(i18n("Failed to create dir: %1", name), icon, i18n("Project"), MessageType::Error);
        return;
    }

    KateProjectItem *i = new KateProjectItem(KateProjectItem::Directory, name, fullDirName);
    item->appendRow(i);
    item->sortChildren(0);
}

void KateProjectViewTree::removeFile(const QModelIndex &idx, const QString &fullFilePath)
{
    auto proxyModel = static_cast<QSortFilterProxyModel *>(model());
    auto index = proxyModel->mapToSource(idx);
    auto item = m_project->model()->itemFromIndex(index);
    if (!item) {
        return;
    }
    QStandardItem *parent = item->parent();

    /**
     * Delete file
     */
    QFile file(fullFilePath);
    if (file.remove()) //.moveToTrash()
    {
        if (parent != nullptr) {
            parent->removeRow(item->row());
            parent->sortChildren(0);
        } else {
            m_project->model()->removeRow(item->row());
            m_project->model()->sort(0);
        }
        m_project->removeFile(fullFilePath);
    }
}

void KateProjectViewTree::openTerminal(const QString &dirPath)
{
    m_pluginView->openTerminal(dirPath, m_project);
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

    auto proxy = static_cast<QSortFilterProxyModel *>(model());
    for (const auto &path : std::as_const(m_expandedNodes)) {
        QStringList parts = path.split(QStringLiteral("/"), Qt::SkipEmptyParts);
        if (parts.empty()) {
            continue;
        }
        QStandardItem *parent = m_project->itemForPath(path);
        if (parent) {
            auto index = proxy->mapFromSource(m_project->model()->indexFromItem(parent));
            expand(index);
        }
    }

    QMetaObject::invokeMethod(
        this,
        [this] {
            verticalScrollBar()->setValue(m_verticalScrollPosition);
        },
        Qt::QueuedConnection);
}

void KateProjectViewTree::contextMenuEvent(QContextMenuEvent *event)
{
    /**
     * get path file path or don't do anything
     */
    const QModelIndex index = indexAt(event->pos());
    const QString filePath = index.isValid() ? index.data(Qt::UserRole).toString() : m_project->baseDir();
    if (filePath.isEmpty()) {
        QTreeView::contextMenuEvent(event);
        return;
    }

    KateProjectTreeViewContextMenu menu;
    menu.exec(filePath, index, viewport()->mapToGlobal(event->pos()), this);

    event->accept();
}
