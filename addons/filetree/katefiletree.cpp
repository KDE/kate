/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>
   SPDX-FileCopyrightText: 2014 Joseph Wenninger <jowenn@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// BEGIN Includes
#include "katefiletree.h"

#include "filehistorywidget.h"
#include "katefiletreemodel.h"
#include "katefiletreeproxymodel.h"

#include "katefileactions.h"

#include <ktexteditor/application.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>

#include <KApplicationTrader>
#include <KIO/ApplicationLauncherJob>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <kio_version.h>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 98, 0)
#include <KIO/JobUiDelegateFactory>
#else
#include <KIO/JobUiDelegate>
#endif
#include <KIO/OpenFileManagerWindowJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDir>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMimeDatabase>
#include <QStyledItemDelegate>
// END Includes

namespace
{
KTextEditor::Document *docFromIndex(const QModelIndex &index)
{
    return index.data(KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
}

QList<KTextEditor::Document *> docTreeFromIndex(const QModelIndex &index)
{
    return index.data(KateFileTreeModel::DocumentTreeRole).value<QList<KTextEditor::Document *>>();
}

bool closeDocs(const QList<KTextEditor::Document *> &docs)
{
    return KTextEditor::Editor::instance()->application()->closeDocuments(docs);
}

class CloseIconStyleDelegate : public QStyledItemDelegate
{
public:
    CloseIconStyleDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        if (!m_showCloseBtn) {
            return;
        }

        if (index.column() == 1 && option.state & QStyle::State_Enabled && option.state & QStyle::State_MouseOver) {
            const QIcon icon = QIcon::fromTheme(QStringLiteral("tab-close"));
            const int w = option.decorationSize.width();
            QRect iconRect(option.rect.right() - w, option.rect.top(), w, option.rect.height());
            icon.paint(painter, iconRect, Qt::AlignRight | Qt::AlignVCenter);
        }
    }

    void setShowCloseButton(bool s)
    {
        m_showCloseBtn = s;
    }

private:
    bool m_showCloseBtn = false;
};
} // namespace

// BEGIN KateFileTree

KateFileTree::KateFileTree(QWidget *parent)
    : QTreeView(parent)
{
    setIndentation(12);
    setAllColumnsShowFocus(true);
    setFocusPolicy(Qt::NoFocus);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    // for hover close button
    viewport()->setAttribute(Qt::WA_Hover);

    // DND
    setDefaultDropAction(Qt::MoveAction);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(false);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragEnabled(true);
    setUniformRowHeights(true);

    setItemDelegate(new CloseIconStyleDelegate(this));

    // handle activated (e.g. for pressing enter) + clicked (to avoid to need to do double-click e.g. on Windows)
    connect(this, &KateFileTree::activated, this, &KateFileTree::mouseClicked);
    connect(this, &KateFileTree::clicked, this, &KateFileTree::mouseClicked);

    m_filelistReloadDocument = new QAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18nc("@action:inmenu", "Reloa&d"), this);
    connect(m_filelistReloadDocument, &QAction::triggered, this, &KateFileTree::slotDocumentReload);
    m_filelistReloadDocument->setWhatsThis(i18n("Reload selected document(s) from disk."));

    m_filelistCloseDocument = new QAction(QIcon::fromTheme(QStringLiteral("document-close")), i18nc("@action:inmenu", "Close"), this);
    connect(m_filelistCloseDocument, &QAction::triggered, this, &KateFileTree::slotDocumentClose);
    m_filelistCloseDocument->setWhatsThis(i18n("Close the current document."));

    m_filelistExpandRecursive = new QAction(QIcon::fromTheme(QStringLiteral("view-list-tree")), i18nc("@action:inmenu", "Expand Recursively"), this);
    connect(m_filelistExpandRecursive, &QAction::triggered, this, &KateFileTree::slotExpandRecursive);
    m_filelistExpandRecursive->setWhatsThis(i18n("Expand the file list sub tree recursively."));

    m_filelistCollapseRecursive = new QAction(QIcon::fromTheme(QStringLiteral("view-list-tree")), i18nc("@action:inmenu", "Collapse Recursively"), this);
    connect(m_filelistCollapseRecursive, &QAction::triggered, this, &KateFileTree::slotCollapseRecursive);
    m_filelistCollapseRecursive->setWhatsThis(i18n("Collapse the file list sub tree recursively."));

    m_filelistCloseOtherDocument = new QAction(QIcon::fromTheme(QStringLiteral("document-close")), i18nc("@action:inmenu", "Close Other"), this);
    connect(m_filelistCloseOtherDocument, &QAction::triggered, this, &KateFileTree::slotDocumentCloseOther);
    m_filelistCloseOtherDocument->setWhatsThis(i18n("Close other documents in this folder."));

    m_filelistOpenContainingFolder =
        new QAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18nc("@action:inmenu", "Open Containing Folder"), this);
    connect(m_filelistOpenContainingFolder, &QAction::triggered, this, &KateFileTree::slotOpenContainingFolder);
    m_filelistOpenContainingFolder->setWhatsThis(i18n("Open the folder this file is located in."));

    m_filelistCopyFilename = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy-path")), i18nc("@action:inmenu", "Copy Location"), this);
    connect(m_filelistCopyFilename, &QAction::triggered, this, &KateFileTree::slotCopyFilename);
    m_filelistCopyFilename->setWhatsThis(i18n("Copy path and filename to the clipboard."));

    m_filelistRenameFile = new QAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18nc("@action:inmenu", "Rename..."), this);
    connect(m_filelistRenameFile, &QAction::triggered, this, &KateFileTree::slotRenameFile);
    m_filelistRenameFile->setWhatsThis(i18n("Rename the selected file."));

    m_filelistPrintDocument = KStandardAction::print(this, &KateFileTree::slotPrintDocument, this);
    m_filelistPrintDocument->setWhatsThis(i18n("Print selected document."));

    m_filelistPrintDocumentPreview = KStandardAction::printPreview(this, &KateFileTree::slotPrintDocumentPreview, this);
    m_filelistPrintDocumentPreview->setWhatsThis(i18n("Show print preview of current document"));

    m_filelistDeleteDocument = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@action:inmenu", "Delete"), this);
    connect(m_filelistDeleteDocument, &QAction::triggered, this, &KateFileTree::slotDocumentDelete);
    m_filelistDeleteDocument->setWhatsThis(i18n("Close and delete selected file from storage."));

    setupContextMenuActionGroups();

    m_resetHistory = new QAction(QIcon::fromTheme(QStringLiteral("edit-clear-history")), i18nc("@action:inmenu", "Clear History"), this);
    connect(m_resetHistory, &QAction::triggered, this, &KateFileTree::slotResetHistory);
    m_resetHistory->setWhatsThis(i18n("Clear edit/view history."));

    QPalette p = palette();
    p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
    p.setColor(QPalette::Inactive, QPalette::HighlightedText, p.color(QPalette::Active, QPalette::HighlightedText));
    setPalette(p);
}

KateFileTree::~KateFileTree() = default;

void KateFileTree::setModel(QAbstractItemModel *model)
{
    m_proxyModel = static_cast<KateFileTreeProxyModel *>(model);
    Q_ASSERT(m_proxyModel); // we don't really work with anything else
    QTreeView::setModel(model);
    m_sourceModel = static_cast<KateFileTreeModel *>(m_proxyModel->sourceModel());

    header()->hide();
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);

    const int minSize = m_hasCloseButton ? 16 : 1;
    header()->setMinimumSectionSize(minSize);
    header()->setSectionResizeMode(1, QHeaderView::Fixed);
    header()->resizeSection(1, minSize);

    // proxy never emits rowsMoved
    connect(m_proxyModel->sourceModel(), &QAbstractItemModel::rowsMoved, this, &KateFileTree::onRowsMoved);
}

void KateFileTree::onRowsMoved(const QModelIndex &, int, int, const QModelIndex &destination, int row)
{
    QModelIndex movedIndex = m_proxyModel->mapFromSource(m_sourceModel->index(row, 0, destination));
    // We moved stuff, make sure if child was expanded, we expand all parents too.
    if (movedIndex.isValid() && isExpanded(movedIndex) && !isExpanded(movedIndex.parent())) {
        QModelIndex movedParent = movedIndex.parent();
        while (movedParent.isValid() && !isExpanded(movedParent)) {
            expand(movedParent);
            movedParent = movedParent.parent();
        }
    }
}

void KateFileTree::setShowCloseButton(bool show)
{
    m_hasCloseButton = show;
    static_cast<CloseIconStyleDelegate *>(itemDelegate())->setShowCloseButton(show);

    if (!header())
        return;

    const int minSize = show ? 16 : 1;
    header()->setMinimumSectionSize(minSize);
    header()->resizeSection(1, minSize);
    header()->viewport()->update();
}

void KateFileTree::setupContextMenuActionGroups()
{
    QActionGroup *modeGroup = new QActionGroup(this);

    m_treeModeAction = setupOption(modeGroup,
                                   QIcon::fromTheme(QStringLiteral("view-list-tree")),
                                   i18nc("@action:inmenu", "Tree Mode"),
                                   i18n("Set view style to Tree Mode"),
                                   &KateFileTree::slotTreeMode,
                                   Qt::Checked);

    m_listModeAction = setupOption(modeGroup,
                                   QIcon::fromTheme(QStringLiteral("view-list-text")),
                                   i18nc("@action:inmenu", "List Mode"),
                                   i18n("Set view style to List Mode"),
                                   &KateFileTree::slotListMode);

    QActionGroup *sortGroup = new QActionGroup(this);

    m_sortByFile = setupOption(sortGroup,
                               QIcon(),
                               i18nc("@action:inmenu sorting option", "Document Name"),
                               i18n("Sort by Document Name"),
                               &KateFileTree::slotSortName,
                               Qt::Checked);

    m_sortByPath =
        setupOption(sortGroup, QIcon(), i18nc("@action:inmenu sorting option", "Document Path"), i18n("Sort by Document Path"), &KateFileTree::slotSortPath);

    m_sortByOpeningOrder = setupOption(sortGroup,
                                       QIcon(),
                                       i18nc("@action:inmenu sorting option", "Opening Order"),
                                       i18n("Sort by Opening Order"),
                                       &KateFileTree::slotSortOpeningOrder);

    m_customSorting = new QAction(QIcon(), i18n("Custom Sorting"), this);
    m_customSorting->setCheckable(true);
    m_customSorting->setActionGroup(sortGroup);
    connect(m_customSorting, &QAction::triggered, this, [this] {
        Q_EMIT sortRoleChanged(CustomSorting);
    });
}

QAction *KateFileTree::setupOption(QActionGroup *group,
                                   const QIcon &icon,
                                   const QString &text,
                                   const QString &whatsThis,
                                   const Func &slot,
                                   Qt::CheckState checked /* = Qt::Unchecked */)
{
    QAction *new_action = new QAction(icon, text, this);
    new_action->setWhatsThis(whatsThis);
    new_action->setActionGroup(group);
    new_action->setCheckable(true);
    new_action->setChecked(checked == Qt::Checked);
    connect(new_action, &QAction::triggered, this, slot);
    return new_action;
}

void KateFileTree::slotListMode()
{
    Q_EMIT viewModeChanged(true);
}

void KateFileTree::slotTreeMode()
{
    Q_EMIT viewModeChanged(false);
}

void KateFileTree::slotSortName()
{
    Q_EMIT sortRoleChanged(Qt::DisplayRole);
}

void KateFileTree::slotSortPath()
{
    Q_EMIT sortRoleChanged(KateFileTreeModel::PathRole);
}

void KateFileTree::slotSortOpeningOrder()
{
    Q_EMIT sortRoleChanged(KateFileTreeModel::OpeningOrderRole);
}

void KateFileTree::slotCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (!current.isValid()) {
        return;
    }

    KTextEditor::Document *doc = m_proxyModel->docFromIndex(current);
    if (doc) {
        m_previouslySelected = current;
    }
}

void KateFileTree::closeClicked(const QModelIndex &index)
{
    if (m_proxyModel->isDir(index)) {
        const QList<KTextEditor::Document *> list = m_proxyModel->docTreeFromIndex(index);
        closeDocs(list);
        return;
    } else if (m_proxyModel->isWidgetDir(index)) {
        const auto idx = index.siblingAtColumn(0);
        const auto count = m_proxyModel->rowCount(idx);
        QWidgetList widgets;
        widgets.reserve(count);
        for (int i = 0; i < count; ++i) {
            widgets << m_proxyModel->index(i, 0, idx).data(KateFileTreeModel::WidgetRole).value<QWidget *>();
        }

        for (const auto &w : widgets) {
            closeWidget(w);
        }
    }

    if (auto *doc = m_proxyModel->docFromIndex(index)) {
        closeDocs({doc});
    } else if (auto *w = index.data(KateFileTreeModel::WidgetRole).value<QWidget *>()) {
        Q_EMIT closeWidget(w);
    }
}

void KateFileTree::mouseClicked(const QModelIndex &index)
{
    if (m_hasCloseButton && index.column() == 1) {
        closeClicked(index);
        return;
    }

    if (auto *doc = m_proxyModel->docFromIndex(index)) {
        Q_EMIT activateDocument(doc);
    } else if (auto *w = index.data(KateFileTreeModel::WidgetRole).value<QWidget *>()) {
        Q_EMIT activateWidget(w);
    }
}

void KateFileTree::contextMenuEvent(QContextMenuEvent *event)
{
    m_indexContextMenu = indexAt(event->pos());
    if (m_indexContextMenu.isValid()) {
        selectionModel()->setCurrentIndex(m_indexContextMenu, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    const bool listMode = m_sourceModel->listMode();
    m_treeModeAction->setChecked(!listMode);
    m_listModeAction->setChecked(listMode);

    const int sortRole = m_proxyModel->sortRole();
    m_sortByFile->setChecked(sortRole == Qt::DisplayRole);
    m_sortByPath->setChecked(sortRole == KateFileTreeModel::PathRole);
    m_sortByOpeningOrder->setChecked(sortRole == KateFileTreeModel::OpeningOrderRole);
    m_customSorting->setChecked(sortRole == CustomSorting);

    KTextEditor::Document *doc = docFromIndex(m_indexContextMenu);

    bool isDir = m_proxyModel->isDir(m_indexContextMenu);
    bool isWidgetDir = m_proxyModel->isWidgetDir(m_indexContextMenu);
    bool isWidget = m_indexContextMenu.data(KateFileTreeModel::WidgetRole).value<QWidget *>() != nullptr;

    QMenu menu(this);
    if (doc) {
        if (doc->url().isValid()) {
            QMenu *openWithMenu = menu.addMenu(i18nc("@action:inmenu", "Open With"));
            openWithMenu->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));
            connect(openWithMenu, &QMenu::aboutToShow, this, [this, openWithMenu]() {
                slotFixOpenWithMenu(openWithMenu);
            });
            connect(openWithMenu, &QMenu::triggered, this, &KateFileTree::slotOpenWithMenuAction);

            menu.addSeparator();
            menu.addAction(m_filelistCopyFilename);
            menu.addAction(m_filelistRenameFile);
            menu.addAction(m_filelistDeleteDocument);
            menu.addAction(m_filelistReloadDocument);

            if (doc->url().isLocalFile()) {
                auto a = menu.addAction(i18n("Show File Git History"));
                connect(a, &QAction::triggered, this, [doc] {
                    auto url = doc->url();
                    if (url.isValid() && url.isLocalFile()) {
                        FileHistory::showFileHistory(url.toLocalFile());
                    }
                });
            }

            menu.addSeparator();
            menu.addAction(m_filelistOpenContainingFolder);

            menu.addSeparator();
            menu.addAction(m_filelistCloseDocument);
            menu.addAction(m_filelistCloseOtherDocument);

            menu.addSeparator();
            menu.addAction(m_filelistPrintDocument);
            menu.addAction(m_filelistPrintDocumentPreview);
        } else {
            // untitled documents
            menu.addAction(m_filelistCloseDocument);

            menu.addSeparator();
        }
    } else if (isDir || isWidgetDir || isWidget) {
        if (isDir) {
            menu.addAction(m_filelistReloadDocument);
        }

        menu.addSeparator();
        menu.addAction(m_filelistCloseDocument);

        menu.addSeparator();
        menu.addAction(m_filelistExpandRecursive);
        menu.addAction(m_filelistCollapseRecursive);
    }

    menu.addSeparator();
    QMenu *view_menu = menu.addMenu(i18nc("@action:inmenu", "View Mode"));
    view_menu->addAction(m_treeModeAction);
    view_menu->addAction(m_listModeAction);

    QMenu *sort_menu = menu.addMenu(QIcon::fromTheme(QStringLiteral("view-sort")), i18nc("@action:inmenu", "Sort By"));
    sort_menu->addAction(m_sortByFile);
    sort_menu->addAction(m_sortByPath);
    sort_menu->addAction(m_sortByOpeningOrder);
    sort_menu->addAction(m_customSorting);

    m_filelistCloseDocument->setEnabled(m_indexContextMenu.isValid());

    menu.addAction(m_resetHistory);

    menu.exec(viewport()->mapToGlobal(event->pos()));

    if (m_previouslySelected.isValid()) {
        selectionModel()->setCurrentIndex(m_previouslySelected, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }

    event->accept();
}

void KateFileTree::slotFixOpenWithMenu(QMenu *menu)
{
    menu->clear();

    KTextEditor::Document *doc = m_proxyModel->docFromIndex(m_indexContextMenu);
    if (!doc) {
        return;
    }

    // get a list of appropriate services.
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(doc->mimeType());

    const KService::List offers = KApplicationTrader::queryByMimeType(mime.name());
    // for each one, insert a menu item...
    for (const auto &service : offers) {
        if (service->name() == QLatin1String("Kate")) {
            continue;
        }
        QAction *a = menu->addAction(QIcon::fromTheme(service->icon()), service->name());
        a->setData(service->entryPath());
    }
    // append "Other..." to call the KDE "open with" dialog.
    QAction *other = menu->addAction(i18n("&Other..."));
    other->setData(QString());
}

void KateFileTree::slotOpenWithMenuAction(QAction *a)
{
    KTextEditor::Document *doc = m_proxyModel->docFromIndex(m_indexContextMenu);
    if (!doc) {
        return;
    }

    const QList<QUrl> list({doc->url()});

    KService::Ptr app = KService::serviceByDesktopPath(a->data().toString());
    // If app is null, ApplicationLauncherJob will invoke the open-with dialog
    auto *job = new KIO::ApplicationLauncherJob(app);
    job->setUrls(list);
#if KIO_VERSION >= QT_VERSION_CHECK(5, 98, 0)
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
#else
    job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
#endif
    job->start();
}

void KateFileTree::slotDocumentClose()
{
    m_previouslySelected = QModelIndex();
    if (!m_indexContextMenu.isValid()) {
        return;
    }
    const auto closeColumnIndex = m_indexContextMenu.sibling(m_indexContextMenu.row(), 1);
    closeClicked(closeColumnIndex);
}

void KateFileTree::addChildrenTolist(const QModelIndex &index, QList<QPersistentModelIndex> *worklist)
{
    const int count = m_proxyModel->rowCount(index);
    worklist->reserve(worklist->size() + count);
    for (int i = 0; i < count; ++i) {
        worklist->append(m_proxyModel->index(i, 0, index));
    }
}

void KateFileTree::slotExpandRecursive()
{
    if (!m_indexContextMenu.isValid()) {
        return;
    }

    // Work list for DFS walk over sub tree
    QList<QPersistentModelIndex> worklist = {m_indexContextMenu};

    while (!worklist.isEmpty()) {
        QPersistentModelIndex index = worklist.takeLast();

        // Expand current item
        expand(index);

        // Append all children of current item
        addChildrenTolist(index, &worklist);
    }
}

void KateFileTree::slotCollapseRecursive()
{
    if (!m_indexContextMenu.isValid()) {
        return;
    }

    // Work list for DFS walk over sub tree
    QList<QPersistentModelIndex> worklist = {m_indexContextMenu};

    while (!worklist.isEmpty()) {
        QPersistentModelIndex index = worklist.takeLast();

        // Expand current item
        collapse(index);

        // Prepend all children of current item
        addChildrenTolist(index, &worklist);
    }
}

void KateFileTree::slotDocumentCloseOther()
{
    QList<KTextEditor::Document *> closingDocuments = m_proxyModel->docTreeFromIndex(m_indexContextMenu.parent());
    KTextEditor::Document *doc = m_proxyModel->docFromIndex(m_indexContextMenu);
    closingDocuments.removeOne(doc);
    closeDocs(closingDocuments);
}

void KateFileTree::slotDocumentReload()
{
    const QList<KTextEditor::Document *> docs = docTreeFromIndex(m_indexContextMenu);
    for (auto *doc : docs) {
        doc->documentReload();
    }
}

void KateFileTree::slotOpenContainingFolder()
{
    KTextEditor::Document *doc = m_proxyModel->docFromIndex(m_indexContextMenu);
    if (doc) {
        KIO::highlightInFileManager({doc->url()});
    }
}

void KateFileTree::slotCopyFilename()
{
    KTextEditor::Document *doc = m_proxyModel->docFromIndex(m_indexContextMenu);

    // TODO: the following code was improved in kate/katefileactions.cpp and should be reused here
    //       (make sure that the mentioned bug 381052 does not reappear)

    if (doc) {
        const QUrl url = doc->url();
        // ensure we prefer native separators, bug 381052
        QApplication::clipboard()->setText(url.isLocalFile() ? QDir::toNativeSeparators(url.toLocalFile()) : url.url());
    }
}

void KateFileTree::slotRenameFile()
{
    KateFileActions::renameDocumentFile(this, m_proxyModel->docFromIndex(m_indexContextMenu));
}

void KateFileTree::slotDocumentFirst()
{
    KTextEditor::Document *doc = m_proxyModel->docFromIndex(m_proxyModel->index(0, 0));
    if (doc) {
        Q_EMIT activateDocument(doc);
    }
}

void KateFileTree::slotDocumentLast()
{
    int count = m_proxyModel->rowCount(m_proxyModel->parent(currentIndex()));
    KTextEditor::Document *doc = m_proxyModel->docFromIndex(m_proxyModel->index(count - 1, 0));
    if (doc) {
        Q_EMIT activateDocument(doc);
    }
}

void KateFileTree::slotDocumentPrev()
{
    QModelIndex current_index = currentIndex();
    QModelIndex prev;

    // scan up the tree skipping any dir nodes
    while (current_index.isValid()) {
        if (current_index.row() > 0) {
            current_index = m_proxyModel->sibling(current_index.row() - 1, current_index.column(), current_index);
            if (!current_index.isValid()) {
                break;
            }

            if (m_proxyModel->isDir(current_index)) {
                // try and select the last child in this parent
                int children = m_proxyModel->rowCount(current_index);
                current_index = m_proxyModel->index(children - 1, 0, current_index);
                if (m_proxyModel->isDir(current_index)) {
                    // since we're a dir, keep going
                    while (m_proxyModel->isDir(current_index)) {
                        children = m_proxyModel->rowCount(current_index);
                        current_index = m_proxyModel->index(children - 1, 0, current_index);
                    }

                    if (!m_proxyModel->isDir(current_index)) {
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
            current_index = m_proxyModel->parent(current_index);
            if (!current_index.isValid()) {
                // paste the root node here, try and wrap around

                int children = m_proxyModel->rowCount(current_index);
                QModelIndex last_index = m_proxyModel->index(children - 1, 0, current_index);
                if (!last_index.isValid()) {
                    break;
                }

                if (m_proxyModel->isDir(last_index)) {
                    // last node is a dir, select last child row
                    int last_children = m_proxyModel->rowCount(last_index);
                    prev = m_proxyModel->index(last_children - 1, 0, last_index);
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
        if (auto *doc = m_proxyModel->docFromIndex(prev)) {
            Q_EMIT activateDocument(doc);
        } else if (auto *w = prev.data(KateFileTreeModel::WidgetRole).value<QWidget *>()) {
            Q_EMIT activateWidget(w);
        }
    }
}

void KateFileTree::slotDocumentNext()
{
    QModelIndex current_index = currentIndex();
    int parent_row_count = m_proxyModel->rowCount(m_proxyModel->parent(current_index));
    QModelIndex next;

    // scan down the tree skipping any dir nodes
    while (current_index.isValid()) {
        if (current_index.row() < parent_row_count - 1) {
            current_index = m_proxyModel->sibling(current_index.row() + 1, current_index.column(), current_index);
            if (!current_index.isValid()) {
                break;
            }

            if (m_proxyModel->isDir(current_index)) {
                // we have a dir node
                while (m_proxyModel->isDir(current_index)) {
                    current_index = m_proxyModel->index(0, 0, current_index);
                }

                parent_row_count = m_proxyModel->rowCount(m_proxyModel->parent(current_index));

                if (!m_proxyModel->isDir(current_index)) {
                    next = current_index;
                    break;
                }
            } else { // found document item
                next = current_index;
                break;
            }
        } else {
            // select the parent's next sibling
            QModelIndex parent_index = m_proxyModel->parent(current_index);
            int grandparent_row_count = m_proxyModel->rowCount(m_proxyModel->parent(parent_index));

            current_index = parent_index;
            parent_row_count = grandparent_row_count;

            // at least if we're not past the last node
            if (!current_index.isValid()) {
                // paste the root node here, try and wrap around
                QModelIndex last_index = m_proxyModel->index(0, 0, QModelIndex());
                if (!last_index.isValid()) {
                    break;
                }

                if (m_proxyModel->isDir(last_index)) {
                    // last node is a dir, select first child row
                    while (m_proxyModel->isDir(last_index)) {
                        if (m_proxyModel->rowCount(last_index)) {
                            // has children, select first
                            last_index = m_proxyModel->index(0, 0, last_index);
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
        if (auto *doc = m_proxyModel->docFromIndex(next)) {
            Q_EMIT activateDocument(doc);
        } else if (auto *w = next.data(KateFileTreeModel::WidgetRole).value<QWidget *>()) {
            Q_EMIT activateWidget(w);
        }
    }
}

void KateFileTree::slotPrintDocument()
{
    KTextEditor::Document *doc = m_proxyModel->docFromIndex(m_indexContextMenu);

    if (!doc) {
        return;
    }

    doc->print();
}

void KateFileTree::slotPrintDocumentPreview()
{
    KTextEditor::Document *doc = m_proxyModel->docFromIndex(m_indexContextMenu);

    if (!doc) {
        return;
    }

    doc->printPreview();
}

void KateFileTree::slotResetHistory()
{
    m_sourceModel->resetHistory();
}

void KateFileTree::slotDocumentDelete()
{
    KTextEditor::Document *doc = m_proxyModel->docFromIndex(m_indexContextMenu);

    // TODO: the following code was improved in kate/katefileactions.cpp and should be reused here

    if (!doc) {
        return;
    }

    QUrl url = doc->url();

    bool go = (KMessageBox::warningContinueCancel(this,
                                                  i18n("Do you really want to delete file \"%1\" from storage?", url.toDisplayString()),
                                                  i18n("Delete file?"),
                                                  KStandardGuiItem::del(),
                                                  KStandardGuiItem::cancel(),
                                                  QStringLiteral("filetreedeletefile"))
               == KMessageBox::Continue);

    if (!go) {
        return;
    }

    if (!closeDocs({doc})) {
        return; // no extra message, the internals of ktexteditor should take care of that.
    }

    if (url.isValid()) {
        KIO::DeleteJob *job = KIO::del(url);
        if (!job->exec()) {
            KMessageBox::error(this, i18n("File \"%1\" could not be deleted.", url.toDisplayString()));
        }
    }
}

// END KateFileTree
