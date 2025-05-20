/*
    SPDX-FileCopyrightText: 2025 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "bookmarksplugin.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>

#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

K_PLUGIN_FACTORY_WITH_JSON(BookmarksPluginFactory, "bookmarksplugin.json", registerPlugin<BookmarksPlugin>();)

BookmarksPlugin::BookmarksPlugin(QObject *parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
{
}

BookmarksPlugin::~BookmarksPlugin() = default;

QObject *BookmarksPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new BookmarksPluginView(this, mainWindow);
}

BookmarksPluginView::BookmarksPluginView(BookmarksPlugin *plugin, KTextEditor::MainWindow *mainWindow)
    : QObject(plugin)
    , m_metaInfos(QStringLiteral("katemetainfos"))
    , m_model()
    , m_selectionModel(&m_model)
    , m_mainWindow(mainWindow)
{
    m_toolView.reset(m_mainWindow->createToolView(plugin,
                                                  QStringLiteral("bookmarks"),
                                                  KTextEditor::MainWindow::Bottom,
                                                  QIcon::fromTheme(QStringLiteral("bookmarks")),
                                                  i18n("Bookmarks")));

    auto root = new QWidget(m_toolView.get());

    QStringList labels = {i18n("Line Number"), i18n("File Location"), QStringLiteral("Id")};
    m_model.setHorizontalHeaderLabels(labels);

    m_treeView = new QTreeView(root);
    m_treeView->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setSortingEnabled(true);
    m_treeView->setModel(&m_model); // TODO: Use custom model?
    m_treeView->setSelectionModel(&m_selectionModel);
    m_treeView->setColumnHidden(2, true);

    auto vLayout = new QVBoxLayout(root);
    vLayout->setSpacing(0);
    vLayout->setContentsMargins({});
    auto hLayout = new QHBoxLayout(root);
    hLayout->setAlignment(QStyle::visualAlignment(Qt::LayoutDirection::LayoutDirectionAuto, Qt::AlignmentFlag::AlignLeft));
    hLayout->setSpacing(0);
    hLayout->setContentsMargins({});

    auto backBtn = new QToolButton(m_treeView);
    backBtn->setEnabled(false);
    backBtn->setToolTip(i18n("Go to Previous Bookmark"));
    backBtn->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    backBtn->setAutoRaise(true);
    connect(backBtn, &QToolButton::clicked, this, &BookmarksPluginView::onBackBtnClicked);
    auto nextBtn = new QToolButton(m_treeView);
    nextBtn->setEnabled(false);
    nextBtn->setToolTip(i18n("Go to Next Bookmark"));
    nextBtn->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    nextBtn->setAutoRaise(true);
    connect(nextBtn, &QToolButton::clicked, this, &BookmarksPluginView::onNextBtnClicked);
    auto removeBtn = new QToolButton(m_treeView);
    removeBtn->setEnabled(false);
    removeBtn->setToolTip(i18n("Remove Bookmark"));
    removeBtn->setIcon(QIcon::fromTheme(QStringLiteral("bookmark-remove")));
    removeBtn->setAutoRaise(true);
    connect(removeBtn, &QToolButton::clicked, this, &BookmarksPluginView::onRemoveBtnClicked);

    vLayout->addLayout(hLayout);
    vLayout->addWidget(m_treeView);
    hLayout->addWidget(backBtn);
    hLayout->addWidget(nextBtn);
    hLayout->addWidget(removeBtn);

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &BookmarksPluginView::onViewChanged);
    connect(&m_selectionModel, &QItemSelectionModel::currentChanged, root, [this, removeBtn](const QModelIndex &current) {
        removeBtn->setEnabled(current.isValid());
        if (current.isValid()) {
            openBookmark(current);
        }
    });

    auto onRowsChanged = [this, backBtn, nextBtn](const QModelIndex &parent) {
        bool enabled = m_model.rowCount(parent) > 0;
        backBtn->setEnabled(enabled);
        nextBtn->setEnabled(enabled);
    };
    connect(&m_model, &QAbstractItemModel::rowsInserted, root, onRowsChanged);
    connect(&m_model, &QAbstractItemModel::rowsRemoved, root, onRowsChanged);

    // Read meta infos and add bookmarks
    for (auto groupName : m_metaInfos.groupList()) {
        KConfigGroup kgroup(&m_metaInfos, groupName);
        QUrl url(kgroup.readEntry("URL"));
        if (!url.isEmpty() && url.isValid()) {
            const QList<int> marks = kgroup.readEntry("Bookmarks", QList<int>());
            for (int i = 0; i < marks.count(); i++) {
                appendBookmark(marks.at(i), url.path());
            }
        }
    }
}

KTextEditor::View *BookmarksPluginView::openBookmark(QModelIndex index)
{
    auto line = index.siblingAtColumn(0).data(Qt::DisplayRole).toInt();
    auto url = index.siblingAtColumn(1).data(Qt::DisplayRole).toUrl();
    auto view = m_mainWindow->openUrl(url);
    view->setCursorPosition(KTextEditor::Cursor(line - 1, 0));
    return view;
}

void BookmarksPluginView::appendDocumentMarks(KTextEditor::Document *document)
{
    auto marks = document->marks();
    for (auto i = marks.cbegin(); i != marks.cend(); ++i) {
        if (i.value()->type == KTextEditor::Document::Bookmark) {
            appendBookmark(i.value()->line, document->url().path());
        }
    }
}

void BookmarksPluginView::onViewChanged(KTextEditor::View *view)
{
    if (view == nullptr || view->document() == nullptr) {
        return;
    }

    appendDocumentMarks(view->document()); // If meta infos are disabled, populate view with runtime marks
    connect(view->document(), &KTextEditor::Document::markChanged, this, &BookmarksPluginView::onMarkChanged, Qt::UniqueConnection);
}

void BookmarksPluginView::onMarkChanged(KTextEditor::Document *document, KTextEditor::Mark mark, KTextEditor::Document::MarkChangeAction action)
{
    if (mark.type == KTextEditor::Document::Bookmark) {
        if (action == KTextEditor::Document::MarkChangeAction::MarkAdded) {
            appendBookmark(mark.line, document->url().path());
        } else if (action == KTextEditor::Document::MarkChangeAction::MarkRemoved) {
            removeBookmark(mark.line, document->url().path());
        }
    }
}

void BookmarksPluginView::onBackBtnClicked()
{
    QModelIndex index = m_treeView->currentIndex();
    int rowCount = m_model.rowCount(index.parent());

    if (rowCount > 0) {
        int prevRow = index.row() - 1;
        if (prevRow < 0) {
            prevRow = rowCount - 1;
        }

        auto prevIndex = m_model.index(prevRow, 0, index.parent());
        m_treeView->setCurrentIndex(prevIndex);
        openBookmark(prevIndex);
    }
}

void BookmarksPluginView::onNextBtnClicked()
{
    QModelIndex index = m_treeView->currentIndex();
    int rowCount = m_model.rowCount(index.parent());

    int nextRow = index.row() + 1;
    if (nextRow >= rowCount) {
        nextRow = 0;
    }

    auto nextIndex = m_model.index(nextRow, 0, index.parent());
    m_treeView->setCurrentIndex(nextIndex);
    openBookmark(nextIndex);
}

void BookmarksPluginView::onRemoveBtnClicked()
{
    QModelIndex index = m_treeView->currentIndex();
    auto line = index.siblingAtColumn(0).data(Qt::DisplayRole).toInt();

    auto view = openBookmark(index);
    if (view == nullptr || view->document() == nullptr) {
        return;
    }

    view->document()->removeMark(line - 1, KTextEditor::Document::Bookmark);
}

void BookmarksPluginView::appendBookmark(int line, QString filepath)
{
    line = line + 1;
    int id = getBookmarkId(line, filepath);
    if (!m_marks.contains(id)) {
        QList<QStandardItem *> items;
        items.append(new QStandardItem(QIcon::fromTheme(QStringLiteral("bookmarks")), QString::number(line)));
        items.append(new QStandardItem(filepath));
        items.append(new QStandardItem(QString::number(id)));
        m_model.invisibleRootItem()->appendRow(items);
        m_marks.insert(id);
    }
}

void BookmarksPluginView::removeBookmark(int line, QString filepath)
{
    line = line + 1;
    int id = getBookmarkId(line, filepath);
    if (m_marks.contains(id)) {
        auto items = m_model.findItems(QString::number(id), Qt::MatchExactly, 2);
        if (!items.empty()) {
            auto row = items.first()->index().row();
            m_model.removeRow(row);
            m_marks.remove(id);
        }
    }
}

int BookmarksPluginView::getBookmarkId(int line, QString filepath)
{
    return std::hash<QString>()(filepath) ^ std::hash<int>()(line);
}

BookmarksPluginView::~BookmarksPluginView()
{
}

#include "bookmarksplugin.moc"
