/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectviewtree.h"
#include "gitwidget.h"
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
#include <QDirIterator>
#include <QPainter>
#include <QProcess>
#include <QScrollBar>
#include <QStyledItemDelegate>

#include <KColorScheme>
#include <KLocalizedString>

#include "drawing_utils.h"
#include <QTextLayout>

#include <KIO/DeleteJob>

namespace
{
struct ActivePart {
    int index;
    int textIndex;
    int textLength;
};

ActivePart activeFlattenedPathItem(const QStringList &splittedPath, int mouseX, int xStart, const QFontMetrics &fm)
{
    int textIndex = 0;
    for (int i = 0; i < splittedPath.size(); ++i) {
        int width = fm.horizontalAdvance(splittedPath[i]);
        if (xStart <= mouseX && mouseX <= xStart + width) {
            return {.index = i, .textIndex = textIndex, .textLength = static_cast<int>(splittedPath[i].size())};
            break;
        }

        xStart += width;
        textIndex += splittedPath[i].length();

        if (i < splittedPath.size() - 1) {
            xStart += fm.horizontalAdvance(QStringLiteral(" / "));
            textIndex += 3;
        }
    }

    return {-1, -1, -1};
}
}

class KateProjectTreeDelegate : public QStyledItemDelegate
{
public:
    KateProjectTreeDelegate(KateProjectViewTree *parent)
        : QStyledItemDelegate(parent)
        , tree(parent)
    {
        KColorScheme c;
        red = c.foreground(KColorScheme::NegativeText).color();
        green = c.foreground(KColorScheme::PositiveText).color();
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const override
    {
        const QString text = index.data().toString();

        // Highlight differently if we have a flattened path
        if (text.contains(u'/')) {
            QStyleOptionViewItem options = opt;
            initStyleOption(&options, index);

            QList<QTextLayout::FormatRange> formats;

            if (opt.state & QStyle::State_MouseOver) {
                const auto mouseX = tree->m_lastMousePosition.x();
                const auto splitted = text.split(QStringLiteral(" / "), Qt::SkipEmptyParts);
                auto textRect = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options, options.widget);
                int xStart = textRect.left();

                ActivePart active = activeFlattenedPathItem(splitted, mouseX, xStart, options.fontMetrics);
                if (active.textIndex != -1) {
                    QTextCharFormat underlineFmt;
                    underlineFmt.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                    formats.append(QTextLayout::FormatRange{active.textIndex, active.textLength, underlineFmt});
                }
            }

            QTextCharFormat fmt;
            fmt.setForeground(options.palette.brush(QPalette::Disabled, QPalette::Text));
            int slashIndex = text.indexOf(u"/");
            while (slashIndex != -1) {
                formats.append(QTextLayout::FormatRange{slashIndex, 1, fmt});
                slashIndex = text.indexOf(u"/", slashIndex + 1);
            }

            painter->save();

            options.text = QString(); // clear text, we'll custom paint it
            options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);
            options.rect.adjust(4, 0, 0, 0);

            auto textRect = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options, options.widget);
            auto width = textRect.x() - options.rect.x();
            painter->translate(width, 0);
            Utils::paintItemViewText(painter, text, options, formats);

            painter->restore();
        } else {
            // Fallback to default
            QStyledItemDelegate::paint(painter, opt, index);
        }

        using StatusType = KateProjectModel::StatusType;
        const auto statusType = index.data(Qt::UserRole + 2).value<StatusType>();
        if (statusType != StatusType::None) {
            painter->save();

            QStyleOptionViewItem option = opt;
            initStyleOption(&option, index);
            QColor color = statusType == StatusType::Added ? green : red;
            painter->setPen(color);
            QRectF circle = option.rect;
            circle.setLeft(option.rect.x() + (circle.width() - (8 + 4)));
            circle.setHeight(8);
            circle.setWidth(8);
            painter->setRenderHint(QPainter::Antialiasing, true);
            circle.moveTop(QStyle::alignedRect(Qt::LayoutDirectionAuto, Qt::AlignVCenter, circle.size().toSize(), option.rect).y());
            painter->setBrush(color);
            painter->drawEllipse(circle);

            painter->restore();
            return;
        }
    }

private:
    QColor red;
    QColor green;
    KateProjectViewTree *const tree;
};

KateProjectViewTree::KateProjectViewTree(KateProjectPluginView *pluginView, KateProject *project)
    : m_pluginView(pluginView)
    , m_project(project)
{
    setMouseTracking(true);
    /**
     * default style
     */
    setHeaderHidden(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAllColumnsShowFocus(true);
    setIndentation(12);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setDragDropMode(QAbstractItemView::DragDrop);
    setDragDropOverwriteMode(false);

    setItemDelegate(new KateProjectTreeDelegate(this));

    /**
     * attach view => project
     * do this once, model is stable for whole project life time
     * kill selection model
     * create sort proxy model
     */
    auto m = selectionModel();
    auto sortModel = new KateProjectFilterProxyModel(this);
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

        flattenPath(index);
    });

    connect(this, &QTreeView::collapsed, this, [this](const QModelIndex &index) {
        QString path = index.data(Qt::UserRole).toString().remove(m_project->baseDir());
        m_expandedNodes.remove(path);
    });
    connect(m_project, &KateProject::projectMapChanged, this, [this] {
        m_verticalScrollPosition = verticalScrollBar()->value();
    });

    connect(m_pluginView->gitWidget(), &GitWidget::statusUpdated, this, [this](const GitUtils::GitParsedStatus &status) {
        if (status.gitRepo.startsWith(m_project->baseDir())) {
            auto proxyModel = static_cast<QSortFilterProxyModel *>(model());
            static_cast<KateProjectModel *>(proxyModel->sourceModel())->setStatus(status);
            viewport()->update();
        }
    });

    /**
     * trigger once some slots
     */
    slotModelChanged();
}

KateProjectViewTree::~KateProjectViewTree() = default;

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
    QPersistentModelIndex index = static_cast<QSortFilterProxyModel *>(model())->mapFromSource(m_project->model()->indexFromItem(item));
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

QStandardItem *KateProjectViewTree::unflattenTreeAndReturnClickedItem(const QModelIndex &idx)
{
    // A directory item with flattened path
    const QStringList splitted = idx.data().toString().split(QStringLiteral(" / "));
    auto rect = visualRect(idx);
    ActivePart active = activeFlattenedPathItem(splitted, m_lastMousePosition.x(), rect.left() + 24, fontMetrics());

    auto proxyModel = static_cast<QSortFilterProxyModel *>(model());
    auto index = proxyModel->mapToSource(idx);
    auto item = m_project->model()->itemFromIndex(index);

    if (active.index < splitted.size() - 1 && active.index >= 0) {
        QString path = item->data(Qt::UserRole).toString();
        QList<QStandardItem *> items;
        for (int i = splitted.size() - 1; i > 0; i--) {
            auto newItem = new KateProjectItem(KateProjectItem::Directory, splitted[i], path);
            if (!items.empty()) {
                newItem->appendRow(items.last());
            }
            items << newItem;

            int lastSlash = path.lastIndexOf(u"/");
            Q_ASSERT(lastSlash != -1);
            path = path.left(lastSlash);
        }
        QStandardItem *innerMost = items.first();
        // Move the files to innerMost item
        innerMost->appendColumn(item->takeColumn(0));
        // Remove all rows
        item->removeRows(0, item->rowCount());
        // Update name and path
        item->setText(splitted[0]);
        item->setData(path, Qt::UserRole);
        // append the top most item
        item->appendRow(items.last());

        items.append(item);

        // Try to flatten the path after we are done adding the file
        QMetaObject::invokeMethod(
            this,
            [this, item] {
                auto proxy = static_cast<QSortFilterProxyModel *>(model());
                auto model = static_cast<QStandardItemModel *>(proxy->sourceModel());
                auto index = model->indexFromItem(item);
                flattenPath(proxy->mapFromSource(index));
            },
            Qt::QueuedConnection);

        std::reverse(items.begin(), items.end());
        return items[active.index];
    }

    return item;
}

void KateProjectViewTree::addFile(const QModelIndex &idx, const QString &fileName)
{
    QStandardItem *item = [idx, this] {
        if (idx.isValid()) {
            if (idx.data().toString().contains(u"/")) {
                return unflattenTreeAndReturnClickedItem(idx);
            }
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
    if (QFileInfo::exists(fullFileName)) {
        Utils::showMessage(i18n("The file already exists"), QIcon(), i18n("Project"), MessageType::Error);
        return;
    }

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

    auto *i = new KateProjectItem(KateProjectItem::File, fileName, fullFileName);
    item->appendRow(i);
    m_project->addFile(fullFileName, i);
    item->sortChildren(0);
}

void KateProjectViewTree::addDirectory(const QModelIndex &idx, const QString &name)
{
    QStandardItem *item = [idx, this] {
        if (idx.isValid()) {
            if (idx.data().toString().contains(u"/")) {
                return unflattenTreeAndReturnClickedItem(idx);
            }
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
    if (QFileInfo::exists(fullDirName)) {
        Utils::showMessage(i18n("The directory already exists"), QIcon(), i18n("Project"), MessageType::Error);
        return;
    }

    QDir dir(base);
    if (!dir.mkdir(name)) {
        const auto icon = QIcon::fromTheme(QStringLiteral("folder-new"));
        Utils::showMessage(i18n("Failed to create dir: %1", name), icon, i18n("Project"), MessageType::Error);
        return;
    }

    auto *i = new KateProjectItem(KateProjectItem::Directory, name, fullDirName);
    item->appendRow(i);
    item->sortChildren(0);
}

void KateProjectViewTree::removePath(const QModelIndex &idx, const QString &path)
{
    auto proxyModel = static_cast<QSortFilterProxyModel *>(model());
    auto index = proxyModel->mapToSource(idx);
    auto item = m_project->model()->itemFromIndex(index);
    if (!item) {
        return;
    }
    QStandardItem *parent = item->parent();

    auto *job = KIO::del(QUrl::fromLocalFile(path));
    if (job->exec()) {
        if (parent != nullptr) {
            parent->removeRow(item->row());
            parent->sortChildren(0);
        } else {
            m_project->model()->removeRow(item->row());
            m_project->model()->sort(0);
        }

        if (QFileInfo(path).isDir()) {
            QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString dir = it.next();
                m_project->removeFile(it.next());
            }
        } else {
            m_project->removeFile(path);
        }

        if (parent) {
            auto parentIdx = m_project->model()->indexFromItem(parent);
            flattenPath(proxyModel->mapFromSource(parentIdx));
        }
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

    KateProjectTreeViewContextMenu::exec(filePath, index, viewport()->mapToGlobal(event->pos()), this);

    event->accept();
}

void KateProjectViewTree::mouseMoveEvent(QMouseEvent *e)
{
    m_lastMousePosition = e->pos();
    if (const auto index = indexAt(e->pos()); index.data().toString().contains(u"/")) {
        update(index);
    }
    QTreeView::mouseMoveEvent(e);
}

KTextEditor::MainWindow *KateProjectViewTree::mainWindow()
{
    return m_pluginView->mainWindow();
}

void KateProjectViewTree::flattenPath(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    auto proxyModel = static_cast<QSortFilterProxyModel *>(model());
    auto sourceIndex = proxyModel->mapToSource(index);
    auto item = static_cast<QStandardItemModel *>(proxyModel->sourceModel())->itemFromIndex(sourceIndex);

    if (item->type() != KateProjectItem::Directory) {
        return;
    }

    // traverse the item as long as it has 1 child
    while (item->rowCount() == 1) {
        auto child = item->child(0);
        if (child && child->type() == KateProjectItem::Directory) {
            item->takeColumn(0);
            const auto childColumn0 = child->takeColumn(0);
            if (!childColumn0.isEmpty()) {
                item->appendColumn(childColumn0);
            }

            // Set the path to child's path
            item->setData(child->data(Qt::UserRole), Qt::UserRole);
            item->setText(QStringLiteral("%1 / %2").arg(item->text(), child->text()));

            delete child;
        } else {
            break;
        }
    }
}
