/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  Copyright (C) 2007 Robert Gruber <rgruber@users.sourceforge.net> 
 *  Copyright (C) 2010 Milian Wolff <mail@milianw.de>
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

#include "snippetview.h"

#include "snippet.h"
#include "katesnippetglobal.h"
#include "snippetrepository.h"
#include "snippetstore.h"
#include "editrepository.h"
#include "editsnippet.h"

#include <KAuthorized>
#include <KLocalizedString>
#include <KMessageBox>

#include <QContextMenuEvent>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QMenu>

#include <kns3/downloaddialog.h>
#include <kns3/uploaddialog.h>


class SnippetFilterModel : public QSortFilterProxyModel {
public:
    SnippetFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) { };
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
        auto index = sourceModel()->index(sourceRow, 0, sourceParent);
        auto item = SnippetStore::self()->itemFromIndex(index);
        if ( ! item ) {
            return false;
        }
        auto snippet = dynamic_cast<Snippet*>(item);
        if ( ! snippet ) {
            return true;
        }
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
};

void SnippetView::setupActionsForWindow(QWidget* widget)
{
    const auto& model = SnippetStore::self();
    for ( int i = 0; i < model->rowCount(); i++ ) {
        auto index = model->index(i, 0, QModelIndex());
        auto item = model->itemFromIndex(index);
        auto repo = dynamic_cast<SnippetRepository*>(item);
        if ( ! repo ) {
            continue;
        }
        for ( int j = 0; j < model->rowCount(index); j++ ) {
            auto item = model->itemFromIndex(model->index(j, 0, index));
            auto snippet = dynamic_cast<Snippet*>(item);
            if ( ! snippet ) {
                continue;
            }
            snippet->registerActionForView(widget);
        }
    }
}

SnippetView::SnippetView(KateSnippetGlobal* plugin, QWidget* parent)
 : QWidget(parent), Ui::SnippetViewBase(), m_plugin(plugin)
{
    Ui::SnippetViewBase::setupUi(this);

    setWindowTitle(i18n("Snippets"));
    setWindowIcon(QIcon::fromTheme(QLatin1String("document-new"), windowIcon()));

    snippetTree->setContextMenuPolicy( Qt::CustomContextMenu );
    snippetTree->viewport()->installEventFilter( this );
    connect(snippetTree, &QTreeView::customContextMenuRequested, this, &SnippetView::contextMenu);

    m_proxy = new SnippetFilterModel(this);
    m_proxy->setFilterKeyColumn(0);
    m_proxy->setSourceModel(SnippetStore::self());

    connect(filterText, &KLineEdit::textChanged, m_proxy, &QSortFilterProxyModel::setFilterFixedString);

    snippetTree->setModel(m_proxy);
    snippetTree->header()->hide();

    m_addRepoAction = new QAction(QIcon::fromTheme(QLatin1String("folder-new")), i18n("Add Repository"), this);
    connect(m_addRepoAction, &QAction::triggered, this, &SnippetView::slotAddRepo);
    addAction(m_addRepoAction);
    m_editRepoAction = new QAction(QIcon::fromTheme(QLatin1String("folder-txt")), i18n("Edit Repository"), this);
    connect(m_editRepoAction, &QAction::triggered, this, &SnippetView::slotEditRepo);
    addAction(m_editRepoAction);
    m_removeRepoAction = new QAction(QIcon::fromTheme(QLatin1String("edit-delete")), i18n("Remove Repository"), this);
    connect(m_removeRepoAction, &QAction::triggered, this, &SnippetView::slotRemoveRepo);
    addAction(m_removeRepoAction);

    const bool newStuffAllowed = KAuthorized::authorize(QStringLiteral("ghns"));

    m_putNewStuffAction = new QAction(QIcon::fromTheme(QLatin1String("get-hot-new-stuff")), i18n("Publish Repository"), this);
    m_putNewStuffAction->setVisible(newStuffAllowed);
    connect(m_putNewStuffAction, &QAction::triggered, this, &SnippetView::slotSnippetToGHNS);
    addAction(m_putNewStuffAction);

    QAction* separator = new QAction(this);
    separator->setSeparator(true);
    addAction(separator);

    m_addSnippetAction = new QAction(QIcon::fromTheme(QLatin1String("document-new")), i18n("Add Snippet"), this);
    connect(m_addSnippetAction, &QAction::triggered, this, &SnippetView::slotAddSnippet);
    addAction(m_addSnippetAction);
    m_editSnippetAction = new QAction(QIcon::fromTheme(QLatin1String("document-edit")), i18n("Edit Snippet"), this);
    connect(m_editSnippetAction, &QAction::triggered, this, &SnippetView::slotEditSnippet);
    addAction(m_editSnippetAction);
    m_removeSnippetAction = new QAction(QIcon::fromTheme(QLatin1String("document-close")), i18n("Remove Snippet"), this);
    connect(m_removeSnippetAction, &QAction::triggered, this, &SnippetView::slotRemoveSnippet);
    addAction(m_removeSnippetAction);

    addAction(separator);

    m_getNewStuffAction = new QAction(QIcon::fromTheme(QLatin1String("get-hot-new-stuff")), i18n("Get New Snippets"), this);
    m_getNewStuffAction->setVisible(newStuffAllowed);
    connect(m_getNewStuffAction, &QAction::triggered, this, &SnippetView::slotGHNS);
    addAction(m_getNewStuffAction);

    connect(snippetTree->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(validateActions()));
    validateActions();

    connect(snippetTree->model(), &QAbstractItemModel::rowsInserted, this, [this]() { setupActionsForWindow(this); });

    m_proxy->setDynamicSortFilter(true);
    m_proxy->sort(0, Qt::AscendingOrder);
}

void SnippetView::validateActions()
{
    QStandardItem* item = currentItem();

    Snippet* selectedSnippet = dynamic_cast<Snippet*>( item );
    SnippetRepository* selectedRepo = dynamic_cast<SnippetRepository*>( item );

    m_addRepoAction->setEnabled(true);
    m_editRepoAction->setEnabled(selectedRepo);
    m_removeRepoAction->setEnabled(selectedRepo);
    m_putNewStuffAction->setEnabled(selectedRepo);

    m_addSnippetAction->setEnabled(selectedRepo || selectedSnippet);
    m_editSnippetAction->setEnabled(selectedSnippet);
    m_removeSnippetAction->setEnabled(selectedSnippet);
}

QStandardItem* SnippetView::currentItem()
{
    ///TODO: support multiple selected items
    QModelIndex index = snippetTree->currentIndex();
    index = m_proxy->mapToSource(index);
    return SnippetStore::self()->itemFromIndex( index );
}

void SnippetView::slotSnippetClicked (const QModelIndex & index)
{
    QStandardItem* item = SnippetStore::self()->itemFromIndex( m_proxy->mapToSource(index) );
    if (!item)
        return;

    Snippet* snippet = dynamic_cast<Snippet*>( item );
    if (!snippet)
        return;

    m_plugin->insertSnippet( snippet );
}

void SnippetView::contextMenu (const QPoint& pos)
{
    QModelIndex index = snippetTree->indexAt( pos );
    index = m_proxy->mapToSource(index);
    QStandardItem* item = SnippetStore::self()->itemFromIndex( index );
    if (!item) {
        // User clicked into an empty place of the tree
        QMenu menu(this);

        menu.addSection(i18n("Snippets"));

        menu.addAction(m_addRepoAction);
        menu.addAction(m_getNewStuffAction);

        menu.exec(snippetTree->mapToGlobal(pos));
    } else if (Snippet* snippet = dynamic_cast<Snippet*>( item )) {
        QMenu menu(this);
        menu.addSection(i18n("Snippet: %1", snippet->text()));

        menu.addAction(m_editSnippetAction);
        menu.addAction(m_removeSnippetAction);

        menu.exec(snippetTree->mapToGlobal(pos));
    } else if (SnippetRepository* repo = dynamic_cast<SnippetRepository*>( item )) {
        QMenu menu(this);
        menu.addSection(i18n("Repository: %1", repo->text()));

        menu.addAction(m_addSnippetAction);
        menu.addSeparator();

        menu.addAction(m_editRepoAction);
        menu.addAction(m_removeRepoAction);
        menu.addAction(m_putNewStuffAction);

        menu.exec(snippetTree->mapToGlobal(pos));
    }
}

void SnippetView::slotEditSnippet()
{
    QStandardItem* item = currentItem();
    if (!item)
        return;

    Snippet* snippet = dynamic_cast<Snippet*>( item );
    if (!snippet)
        return;

    SnippetRepository* repo = dynamic_cast<SnippetRepository*>( item->parent() );
    if (!repo)
        return;

    EditSnippet dlg(repo, snippet, this);
    dlg.exec();
}

void SnippetView::slotAddSnippet()
{
    QStandardItem* item = currentItem();
    if (!item)
        return;

    SnippetRepository* repo = dynamic_cast<SnippetRepository*>( item );
    if (!repo) {
        repo = dynamic_cast<SnippetRepository*>( item->parent() );
        if ( !repo ) 
            return;
    }

    EditSnippet dlg(repo, nullptr, this);
    dlg.exec();
}

void SnippetView::slotRemoveSnippet()
{
    QStandardItem* item = currentItem();
    if (!item)
        return;

    SnippetRepository* repo = dynamic_cast<SnippetRepository*>( item->parent() );
    if (!repo)
        return;

    int ans = KMessageBox::warningContinueCancel(
        QApplication::activeWindow(),
        i18n("Do you really want to delete the snippet \"%1\"?", item->text())
    );
    if ( ans == KMessageBox::Continue ) {
        item->parent()->removeRow(item->row());
        repo->save();
    }
}

void SnippetView::slotAddRepo()
{
    EditRepository dlg(nullptr, this);
    dlg.exec();
}

void SnippetView::slotEditRepo()
{
    QStandardItem* item = currentItem();
    if (!item)
        return;

    SnippetRepository* repo = dynamic_cast<SnippetRepository*>( item );
    if (!repo)
        return;

    EditRepository dlg(repo, this);
    dlg.exec();
}

void SnippetView::slotRemoveRepo()
{
    QStandardItem* item = currentItem();
    if (!item)
        return;

    SnippetRepository* repo = dynamic_cast<SnippetRepository*>( item );
    if (!repo)
        return;

    int ans = KMessageBox::warningContinueCancel(
        QApplication::activeWindow(),
        i18n("Do you really want to delete the repository \"%1\" with all its snippets?", repo->text())
    );
    if ( ans == KMessageBox::Continue ) {
        repo->remove();
    }
}

void SnippetView::slotGHNS()
{
    KNS3::DownloadDialog dialog(QLatin1String(":/katesnippets/ktexteditor_codesnippets_core.knsrc"), this);
    dialog.exec();
    foreach ( const KNS3::Entry& entry, dialog.changedEntries() ) {
        foreach ( const QString& path, entry.uninstalledFiles() ) {
            if ( path.endsWith(QLatin1String(".xml")) ) {
                if ( SnippetRepository* repo = SnippetStore::self()->repositoryForFile(path) ) {
                    repo->remove();
                }
            }
        }
        foreach ( const QString& path, entry.installedFiles() ) {
            if ( path.endsWith(QLatin1String(".xml")) ) {
                SnippetStore::self()->appendRow(new SnippetRepository(path));
            }
        }
    }
}

void SnippetView::slotSnippetToGHNS()
{
    QStandardItem* item = currentItem();
    if ( !item)
        return;

    SnippetRepository* repo = dynamic_cast<SnippetRepository*>( item );
    if ( !repo )
        return;

    KNS3::UploadDialog dialog(QLatin1String(":/katesnippets/ktexteditor_codesnippets_core.knsrc"), this);
    dialog.setUploadFile(QUrl::fromLocalFile(repo->file()));
    dialog.setUploadName(repo->text());
    dialog.exec();
}

bool SnippetView::eventFilter(QObject* obj, QEvent* e)
{
    // no, listening to activated() is not enough since that would also trigger the edit mode which we _dont_ want here
    // users may still rename stuff via select + F2 though
    if (obj == snippetTree->viewport()) {
        const bool singleClick = style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, this);
        if ( (!singleClick && e->type() == QEvent::MouseButtonDblClick) || (singleClick && e->type() == QEvent::MouseButtonRelease) ) {
            QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(e);
            Q_ASSERT(mouseEvent);
            QModelIndex clickedIndex = snippetTree->indexAt(mouseEvent->pos());
            if (clickedIndex.isValid() && clickedIndex.parent().isValid()) {
                slotSnippetClicked(clickedIndex);
                e->accept();
                return true;
            }
        }
    }
    return QObject::eventFilter(obj, e);
}


