/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>
   Copyright (C) 2014 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes
#include "katefiletree.h"

#include "katefiletreemodel.h"
#include "katefiletreeproxymodel.h"
#include "katefiletreedebug.h"

#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/application.h>

#include <KMimeTypeTrader>
#include <KOpenWithDialog>
#include <KRun>
#include <KMessageBox>
#include <KLocalizedString>
#include <KStandardAction>
#include <KIO/DeleteJob>
#include <KIO/CopyJob>

#include <QMimeDatabase>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QApplication>
#include <QMenu>
#include <QInputDialog>
#include <QLineEdit>
#include <QDir>
//END Includes

//BEGIN KateFileTree

KateFileTree::KateFileTree(QWidget *parent): QTreeView(parent)
{
    setAcceptDrops(false);
    setIndentation(12);
    setAllColumnsShowFocus(true);
    setFocusPolicy(Qt::NoFocus);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    // handle activated (e.g. for pressing enter) + clicked (to avoid to need to do double-click e.g. on Windows)
    connect(this, &KateFileTree::activated, this, &KateFileTree::mouseClicked);
    connect(this, &KateFileTree::clicked, this, &KateFileTree::mouseClicked);

    m_filelistReloadDocument = new QAction(QIcon::fromTheme(QLatin1String("view-refresh")), i18nc("@action:inmenu", "Reloa&d"), this);
    connect(m_filelistReloadDocument, &QAction::triggered, this, &KateFileTree::slotDocumentReload);
    m_filelistReloadDocument->setWhatsThis(i18n("Reload selected document(s) from disk."));

    m_filelistCloseDocument = new QAction(QIcon::fromTheme(QLatin1String("document-close")), i18nc("@action:inmenu", "Close"), this);
    connect(m_filelistCloseDocument, &QAction::triggered, this, &KateFileTree::slotDocumentClose);
    m_filelistCloseDocument->setWhatsThis(i18n("Close the current document."));

    m_filelistExpandRecursive = new QAction(QIcon::fromTheme(QLatin1String("view-list-tree")), i18nc("@action:inmenu", "Expand recursively"), this);
    connect(m_filelistExpandRecursive, &QAction::triggered, this, &KateFileTree::slotExpandRecursive);
    m_filelistExpandRecursive->setWhatsThis(i18n("Expand the file list sub tree recursively."));

    m_filelistCollapseRecursive = new QAction(QIcon::fromTheme(QLatin1String("view-list-tree")), i18nc("@action:inmenu", "Collapse recursively"), this);
    connect(m_filelistCollapseRecursive, &QAction::triggered, this, &KateFileTree::slotCollapseRecursive);
    m_filelistCollapseRecursive->setWhatsThis(i18n("Collapse the file list sub tree recursively."));

    m_filelistCloseOtherDocument = new QAction(QIcon::fromTheme(QLatin1String("document-close")), i18nc("@action:inmenu", "Close Other"), this);
    connect(m_filelistCloseOtherDocument, &QAction::triggered, this, &KateFileTree::slotDocumentCloseOther);
    m_filelistCloseOtherDocument->setWhatsThis(i18n("Close other documents in this folder."));

    m_filelistCopyFilename = new QAction(QIcon::fromTheme(QLatin1String("edit-copy")), i18nc("@action:inmenu", "Copy Filename"), this);
    connect(m_filelistCopyFilename, &QAction::triggered, this, &KateFileTree::slotCopyFilename);
    m_filelistCopyFilename->setWhatsThis(i18n("Copy the filename of the file."));

    m_filelistRenameFile = new QAction(QIcon::fromTheme(QLatin1String("edit-rename")), i18nc("@action:inmenu", "Rename File"), this);
    connect(m_filelistRenameFile, &QAction::triggered, this, &KateFileTree::slotRenameFile);
    m_filelistRenameFile->setWhatsThis(i18n("Rename the selected file."));

    m_filelistPrintDocument = KStandardAction::print(this, SLOT(slotPrintDocument()), this);
    m_filelistPrintDocument->setWhatsThis(i18n("Print selected document."));

    m_filelistPrintDocumentPreview = KStandardAction::printPreview(this, SLOT(slotPrintDocumentPreview()), this);
    m_filelistPrintDocumentPreview->setWhatsThis(i18n("Show print preview of current document"));

    m_filelistDeleteDocument = new QAction(QIcon::fromTheme(QLatin1String("edit-delete-shred")), i18nc("@action:inmenu", "Delete Document"), this);
    connect(m_filelistDeleteDocument, &QAction::triggered, this, &KateFileTree::slotDocumentDelete);
    m_filelistDeleteDocument->setWhatsThis(i18n("Close and delete selected file from storage."));

    QActionGroup *modeGroup = new QActionGroup(this);

    m_treeModeAction = setupOption(modeGroup, QIcon::fromTheme(QLatin1String("view-list-tree")), i18nc("@action:inmenu", "Tree Mode"),
                                   i18n("Set view style to Tree Mode"),
                                   SLOT(slotTreeMode()), true);

    m_listModeAction = setupOption(modeGroup, QIcon::fromTheme(QLatin1String("view-list-text")), i18nc("@action:inmenu", "List Mode"),
                                   i18n("Set view style to List Mode"),
                                   SLOT(slotListMode()), false);

    QActionGroup *sortGroup = new QActionGroup(this);

    m_sortByFile = setupOption(sortGroup, QIcon(), i18nc("@action:inmenu sorting option", "Document Name"),
                               i18n("Sort by Document Name"),
                               SLOT(slotSortName()), true);

    m_sortByPath = setupOption(sortGroup, QIcon(), i18nc("@action:inmenu sorting option", "Document Path"),
                               i18n("Sort by Document Path"),
                               SLOT(slotSortPath()), false);

    m_sortByOpeningOrder =  setupOption(sortGroup, QIcon(), i18nc("@action:inmenu sorting option", "Opening Order"),
                                        i18n("Sort by Opening Order"),
                                        SLOT(slotSortOpeningOrder()), false);

    m_resetHistory = new QAction(QIcon::fromTheme(QLatin1String("edit-clear-history")), i18nc("@action:inmenu", "Clear History"), this);
    connect(m_resetHistory, &QAction::triggered, this, &KateFileTree::slotResetHistory);
    m_resetHistory->setWhatsThis(i18n("Clear edit/view history."));

    QPalette p = palette();
    p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText, p.color(QPalette::Active, QPalette::HighlightedText));
    setPalette(p);
}

KateFileTree::~KateFileTree()
{}

void KateFileTree::setModel(QAbstractItemModel *model)
{
    Q_ASSERT(qobject_cast<KateFileTreeProxyModel *>(model)); // we don't really work with anything else
    QTreeView::setModel(model);
}

QAction *KateFileTree::setupOption(
    QActionGroup *group,
    const QIcon &icon,
    const QString &label,
    const QString &whatsThis,
    const char *slot,
    bool checked
)
{
    QAction *new_action = new QAction(icon, label, this);
    new_action->setWhatsThis(whatsThis);
    new_action->setActionGroup(group);
    new_action->setCheckable(true);
    new_action->setChecked(checked);
    connect(new_action, SIGNAL(triggered()), this, slot);
    return new_action;
}

void KateFileTree::slotListMode()
{
    emit viewModeChanged(true);
}

void KateFileTree::slotTreeMode()
{
    emit viewModeChanged(false);
}

void KateFileTree::slotSortName()
{
    emit sortRoleChanged(Qt::DisplayRole);
}

void KateFileTree::slotSortPath()
{
    emit sortRoleChanged(KateFileTreeModel::PathRole);
}

void KateFileTree::slotSortOpeningOrder()
{
    emit sortRoleChanged(KateFileTreeModel::OpeningOrderRole);
}

void KateFileTree::slotCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (!current.isValid()) {
        return;
    }

    KTextEditor::Document *doc = model()->data(current, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    if (doc) {
        m_previouslySelected = current;
    }
}

void KateFileTree::mouseClicked(const QModelIndex &index)
{
    if (auto doc = model()->data(index, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>()) {
        emit activateDocument(doc);
    }
}

void KateFileTree::contextMenuEvent(QContextMenuEvent *event)
{
    m_indexContextMenu = selectionModel()->currentIndex();

    selectionModel()->setCurrentIndex(m_indexContextMenu, QItemSelectionModel::ClearAndSelect);

    KateFileTreeProxyModel *ftpm = static_cast<KateFileTreeProxyModel *>(model());
    KateFileTreeModel *ftm = static_cast<KateFileTreeModel *>(ftpm->sourceModel());

    bool listMode = ftm->listMode();
    m_treeModeAction->setChecked(!listMode);
    m_listModeAction->setChecked(listMode);

    int sortRole = ftpm->sortRole();
    m_sortByFile->setChecked(sortRole == Qt::DisplayRole);
    m_sortByPath->setChecked(sortRole == KateFileTreeModel::PathRole);
    m_sortByOpeningOrder->setChecked(sortRole == KateFileTreeModel::OpeningOrderRole);

    KTextEditor::Document *doc = m_indexContextMenu.data(KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    const bool isFile = (nullptr != doc);

    QMenu menu;
    menu.addAction(m_filelistReloadDocument);
    menu.addAction(m_filelistCloseDocument);
    menu.addAction(m_filelistExpandRecursive);
    menu.addAction(m_filelistCollapseRecursive);

    if (isFile) {
        menu.addAction(m_filelistCloseOtherDocument);
        menu.addSeparator();
        menu.addAction(m_filelistCopyFilename);
        menu.addAction(m_filelistRenameFile);
        menu.addAction(m_filelistPrintDocument);
        menu.addAction(m_filelistPrintDocumentPreview);
        QMenu *openWithMenu = menu.addMenu(i18nc("@action:inmenu", "Open With"));
        connect(openWithMenu, &QMenu::aboutToShow, this, &KateFileTree::slotFixOpenWithMenu);
        connect(openWithMenu, &QMenu::triggered, this, &KateFileTree::slotOpenWithMenuAction);

        const bool hasFileName = doc->url().isValid();
        m_filelistCopyFilename->setEnabled(hasFileName);
        m_filelistRenameFile->setEnabled(hasFileName);
        m_filelistDeleteDocument->setEnabled(hasFileName);
        menu.addAction(m_filelistDeleteDocument);
    }

    menu.addSeparator();
    QMenu *view_menu = menu.addMenu(i18nc("@action:inmenu", "View Mode"));
    view_menu->addAction(m_treeModeAction);
    view_menu->addAction(m_listModeAction);

    QMenu *sort_menu = menu.addMenu(QIcon::fromTheme(QLatin1String("view-sort-ascending")), i18nc("@action:inmenu", "Sort By"));
    sort_menu->addAction(m_sortByFile);
    sort_menu->addAction(m_sortByPath);
    sort_menu->addAction(m_sortByOpeningOrder);

    menu.addAction(m_resetHistory);

    menu.exec(viewport()->mapToGlobal(event->pos()));

    if (m_previouslySelected.isValid()) {
        selectionModel()->setCurrentIndex(m_previouslySelected, QItemSelectionModel::ClearAndSelect);
    }

    event->accept();
}

void KateFileTree::slotFixOpenWithMenu()
{
    QMenu *menu = (QMenu *)sender();
    menu->clear();

    KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    if (!doc) {
        return;
    }

    // get a list of appropriate services.
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(doc->mimeType());

    QAction *a = nullptr;
    KService::List offers = KMimeTypeTrader::self()->query(mime.name(), QLatin1String("Application"));
    // for each one, insert a menu item...
    for (KService::List::Iterator it = offers.begin(); it != offers.end(); ++it) {
        KService::Ptr service = *it;
        if (service->name() == QLatin1String("Kate")) {
            continue;
        }
        a = menu->addAction(QIcon::fromTheme(service->icon()), service->name());
        a->setData(service->entryPath());
    }
    // append "Other..." to call the KDE "open with" dialog.
    a = menu->addAction(i18n("&Other..."));
    a->setData(QString());
}

void KateFileTree::slotOpenWithMenuAction(QAction *a)
{
    QList<QUrl> list;

    KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    if (!doc) {
        return;
    }

    list.append(doc->url());

    const QString openWith = a->data().toString();
    if (openWith.isEmpty()) {
        // display "open with" dialog
        KOpenWithDialog dlg(list);
        if (dlg.exec()) {
            KRun::runService(*dlg.service(), list, this);
        }
        return;
    }

    KService::Ptr app = KService::serviceByDesktopPath(openWith);
    if (app) {
        KRun::runService(*app, list, this);
    } else {
        KMessageBox::error(this, i18n("Application '%1' not found.", openWith), i18n("Application not found"));
    }
}

Q_DECLARE_METATYPE(QList<KTextEditor::Document *>)

void KateFileTree::slotDocumentClose()
{
    m_previouslySelected = QModelIndex();
    QVariant v = m_indexContextMenu.data(KateFileTreeModel::DocumentTreeRole);
    if (!v.isValid()) {
        return;
    }
    QList<KTextEditor::Document *> closingDocuments = v.value<QList<KTextEditor::Document *> >();
    KTextEditor::Editor::instance()->application()->closeDocuments(closingDocuments);
}

void KateFileTree::slotExpandRecursive()
{
    if (! m_indexContextMenu.isValid()) {
        return;
    }

    // Work list for DFS walk over sub tree
    QList<QPersistentModelIndex> worklist = { m_indexContextMenu };

    while (! worklist.isEmpty()) {
        QPersistentModelIndex index = worklist.takeLast();

        // Expand current item
        expand(index);

        // Append all children of current item
        for (int i=0 ; i < model()->rowCount(index) ; ++i) {
            worklist.append(index.child(i,0));
        }
    }
}

void KateFileTree::slotCollapseRecursive()
{
    if (! m_indexContextMenu.isValid()) {
        return;
    }

    // Work list for DFS walk over sub tree
    QList<QPersistentModelIndex> worklist = { m_indexContextMenu };

    while (! worklist.isEmpty()) {
        QPersistentModelIndex index = worklist.takeLast();

        // Expand current item
        collapse(index);

        // Prepend all children of current item
        for (int i=0 ; i < model()->rowCount(index) ; ++i) {
            worklist.append(index.child(i,0));
        }
    }
}

void KateFileTree::slotDocumentCloseOther()
{
    QVariant v = model()->data(m_indexContextMenu.parent(), KateFileTreeModel::DocumentTreeRole);
    if (!v.isValid()) {
        return;
    }

    QList<KTextEditor::Document *> closingDocuments = v.value<QList<KTextEditor::Document *> >();
    KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();

    closingDocuments.removeOne(doc);

    KTextEditor::Editor::instance()->application()->closeDocuments(closingDocuments);
}

void KateFileTree::slotDocumentReload()
{
    QVariant v = m_indexContextMenu.data(KateFileTreeModel::DocumentTreeRole);
    if (!v.isValid()) {
        return;
    }

    QList<KTextEditor::Document *> docs = v.value<QList<KTextEditor::Document *> >();
    foreach(KTextEditor::Document * doc, docs) {
        doc->documentReload();
    }
}

void KateFileTree::slotCopyFilename()
{
    KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    if (doc) {
        // ensure we prefer native separators, bug 381052
        if (doc->url().isLocalFile()) {
            QApplication::clipboard()->setText(QDir::toNativeSeparators(doc->url().toLocalFile()));
        } else {
            QApplication::clipboard()->setText(doc->url().url());
        }
    }
}

void KateFileTree::slotRenameFile() {
    KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    if (!doc) {
        return;
    }

    const QUrl oldFileUrl = doc->url();
    const QString oldFileName = doc->url().fileName();
    bool ok;

    QString newFileName = QInputDialog::getText(this,
        i18n("Rename file"), i18n("New file name"), QLineEdit::Normal, oldFileName, &ok);
    if (!ok) {
        return;
    }

    QUrl newFileUrl = oldFileUrl.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash);
    newFileUrl.setPath(newFileUrl.path() + QLatin1Char('/') + newFileName);

    if (!newFileUrl.isValid()) {
        return;
    }

    if (!doc->closeUrl()) {
        return;
    }

    doc->waitSaveComplete();

    KIO::CopyJob* job = KIO::move(oldFileUrl, newFileUrl);
    QSharedPointer<QMetaObject::Connection> sc(new QMetaObject::Connection());
    auto success = [doc, sc] (KIO::Job*, const QUrl&, const QUrl &realNewFileUrl, const QDateTime&, bool, bool) {
            doc->openUrl(realNewFileUrl);
            doc->documentSavedOrUploaded(doc, true);
            QObject::disconnect(*sc);
        };
    *sc = connect(job, &KIO::CopyJob::copyingDone, doc, success);

    if (!job->exec()) {
        KMessageBox::sorry(this, i18n("File \"%1\" could not be moved to \"%2\"", oldFileUrl.toDisplayString(), newFileUrl.toDisplayString()));
        doc->openUrl(oldFileUrl);
    }
}

void KateFileTree::slotDocumentFirst()
{
    KTextEditor::Document *doc =
        model()->data(model()->index(0, 0),
                      KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    if (doc) {
        emit activateDocument(doc);
    }
}

void KateFileTree::slotDocumentLast()
{
    int count = model()->rowCount(model()->parent(currentIndex()));
    KTextEditor::Document *doc =
        model()->data(model()->index(count - 1, 0),
                      KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    if (doc) {
        emit activateDocument(doc);
    }
}

void KateFileTree::slotDocumentPrev()
{
    KateFileTreeProxyModel *ftpm = static_cast<KateFileTreeProxyModel *>(model());

    QModelIndex current_index = currentIndex();
    QModelIndex prev;

    // scan up the tree skipping any dir nodes
    while (current_index.isValid()) {
        if (current_index.row() > 0) {
            current_index = ftpm->sibling(current_index.row() - 1, current_index.column(), current_index);
            if (!current_index.isValid()) {
                break;
            }

            if (ftpm->isDir(current_index)) {
                // try and select the last child in this parent
                int children = ftpm->rowCount(current_index);
                current_index = ftpm->index(children - 1, 0, current_index);
                if (ftpm->isDir(current_index)) {
                    // since we're a dir, keep going
                    while (ftpm->isDir(current_index)) {
                        children = ftpm->rowCount(current_index);
                        current_index = ftpm->index(children - 1, 0, current_index);
                    }

                    if (!ftpm->isDir(current_index)) {
                        prev = current_index;
                        break;
                    }

                    continue;
                } else {
                    // we're the previous file, set prev
                    prev = current_index;
                    break;
                }
            } else { // found document item
                prev = current_index;
                break;
            }
        } else {
            // just select the parent, the logic above will handle the rest
            current_index = ftpm->parent(current_index);
            if (!current_index.isValid()) {
                // paste the root node here, try and wrap around

                int children = ftpm->rowCount(current_index);
                QModelIndex last_index = ftpm->index(children - 1, 0, current_index);
                if (!last_index.isValid()) {
                    break;
                }

                if (ftpm->isDir(last_index)) {
                    // last node is a dir, select last child row
                    int last_children = ftpm->rowCount(last_index);
                    prev = ftpm->index(last_children - 1, 0, last_index);
                    // bug here?
                    break;
                } else {
                    // got last file node
                    prev = last_index;
                    break;
                }
            }
        }
    }

    if (prev.isValid()) {
        KTextEditor::Document *doc = model()->data(prev, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
        emit activateDocument(doc);
    }
}

void KateFileTree::slotDocumentNext()
{
    KateFileTreeProxyModel *ftpm = static_cast<KateFileTreeProxyModel *>(model());

    QModelIndex current_index = currentIndex();
    int parent_row_count = ftpm->rowCount(ftpm->parent(current_index));
    QModelIndex next;

    // scan down the tree skipping any dir nodes
    while (current_index.isValid()) {
        if (current_index.row() < parent_row_count - 1) {
            current_index = ftpm->sibling(current_index.row() + 1, current_index.column(), current_index);
            if (!current_index.isValid()) {
                break;
            }

            if (ftpm->isDir(current_index)) {
                // we have a dir node
                while (ftpm->isDir(current_index)) {
                    current_index = ftpm->index(0, 0, current_index);
                }

                parent_row_count = ftpm->rowCount(ftpm->parent(current_index));

                if (!ftpm->isDir(current_index)) {
                    next = current_index;
                    break;
                }
            } else { // found document item
                next = current_index;
                break;
            }
        } else {
            // select the parent's next sibling
            QModelIndex parent_index = ftpm->parent(current_index);
            int grandparent_row_count = ftpm->rowCount(ftpm->parent(parent_index));

            current_index = parent_index;
            parent_row_count = grandparent_row_count;

            // at least if we're not past the last node
            if (!current_index.isValid()) {
                // paste the root node here, try and wrap around
                QModelIndex last_index = ftpm->index(0, 0, QModelIndex());
                if (!last_index.isValid()) {
                    break;
                }

                if (ftpm->isDir(last_index)) {
                    // last node is a dir, select first child row
                    while (ftpm->isDir(last_index)) {
                        if (ftpm->rowCount(last_index)) {
                            // has children, select first
                            last_index = ftpm->index(0, 0, last_index);
                        }
                    }

                    next = last_index;
                    break;
                } else {
                    // got first file node
                    next = last_index;
                    break;
                }
            }
        }
    }

    if (next.isValid()) {
        KTextEditor::Document *doc = model()->data(next, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
        emit activateDocument(doc);
    }
}

void KateFileTree::slotPrintDocument()
{
    KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();

    if (!doc) {
        return;
    }

    doc->print();
}

void KateFileTree::slotPrintDocumentPreview()
{
    KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();

    if (!doc) {
        return;
    }

    doc->printPreview();
}

void KateFileTree::slotResetHistory()
{
    KateFileTreeProxyModel *ftpm = static_cast<KateFileTreeProxyModel *>(model());
    KateFileTreeModel *ftm = static_cast<KateFileTreeModel *>(ftpm->sourceModel());
    ftm->resetHistory();
}

void KateFileTree::slotDocumentDelete()
{
    KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();

    if (!doc) {
        return;
    }

    QUrl url = doc->url();

    bool go = (KMessageBox::warningContinueCancel(this,
               i18n("Do you really want to delete file \"%1\" from storage?", url.toDisplayString()),
               i18n("Delete file?"),
               KStandardGuiItem::yes(), KStandardGuiItem::no(), QLatin1String("filetreedeletefile")
                                                 ) == KMessageBox::Continue);

    if (!go) {
        return;
    }

    if (!KTextEditor::Editor::instance()->application()->closeDocument(doc)) {
        return; // no extra message, the internals of ktexteditor should take care of that.
    }

    if (url.isValid()) {
        KIO::DeleteJob *job = KIO::del(url);
        if (!job->exec()) {
            KMessageBox::sorry(this, i18n("File \"%1\" could not be deleted.", url.toDisplayString()));
        }
    }
}

//END KateFileTree

