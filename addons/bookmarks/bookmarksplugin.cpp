/*
    SPDX-FileCopyrightText: 2025 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "bookmarksplugin.h"
#include "bookmarksmodel.h"

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
#include <QSortFilterProxyModel>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

K_PLUGIN_FACTORY_WITH_JSON(BookmarksPluginFactory, "bookmarksplugin.json", registerPlugin<BookmarksPlugin>();)

BookmarksPlugin::~BookmarksPlugin() = default;

BookmarksPlugin::BookmarksPlugin(QObject *parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
    , m_model(parent)
{
    // Read meta infos and add bookmarks
    KConfig metaInfos(QStringLiteral("katemetainfos"));
    for (auto groupName : metaInfos.groupList()) {
        KConfigGroup kgroup(&metaInfos, groupName);
        QUrl url(kgroup.readEntry("URL"));
        if (!url.isEmpty() && url.isValid()) {
            const QList<int> marks = kgroup.readEntry("Bookmarks", QList<int>());
            for (int i = 0; i < marks.count(); i++) {
                m_model.addBookmark(url, marks.at(i));
            }
        }
    }
}

QObject *BookmarksPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new BookmarksPluginView(this, mainWindow, &m_model);
}

BookmarksPluginView::BookmarksPluginView(BookmarksPlugin *plugin, KTextEditor::MainWindow *mainWindow, BookmarksModel *model)
    : QObject(plugin)
    , m_model(model)
    , m_mainWindow(mainWindow)
{
    m_toolView.reset(m_mainWindow->createToolView(plugin,
                                                  QStringLiteral("bookmarks"),
                                                  KTextEditor::MainWindow::Bottom,
                                                  QIcon::fromTheme(QStringLiteral("bookmarks")),
                                                  i18n("Bookmarks")));

    auto root = new QWidget(m_toolView.get());

    m_proxyModel.setSourceModel(model);
    m_selectionModel.setModel(&m_proxyModel);

    m_treeView = new QTreeView(root);
    m_treeView->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setSortingEnabled(true);
    m_treeView->setModel(&m_proxyModel);
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
    backBtn->setEnabled(model->rowCount() > 0);
    backBtn->setToolTip(i18n("Go to Previous Bookmark"));
    backBtn->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    backBtn->setAutoRaise(true);
    connect(backBtn, &QToolButton::clicked, this, &BookmarksPluginView::onBackBtnClicked);
    auto nextBtn = new QToolButton(m_treeView);
    nextBtn->setEnabled(model->rowCount() > 0);
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
    connect(m_treeView, &QTreeView::clicked, this, &BookmarksPluginView::onBookmarkClicked);
    connect(&m_selectionModel, &QItemSelectionModel::selectionChanged, root, [this, removeBtn](const QItemSelection &selected) {
        auto indexes = m_proxyModel.mapSelectionToSource(selected).indexes();
        removeBtn->setEnabled(!indexes.empty());
        if (!indexes.empty()) {
            auto current = indexes.first();
            if (current.isValid()) {
                openBookmark(m_model->getBookmark(current));
            }
        }
    });

    auto onRowsChanged = [model, backBtn, nextBtn](const QModelIndex &parent) {
        bool enabled = model->rowCount(parent) > 1;
        backBtn->setEnabled(enabled);
        nextBtn->setEnabled(enabled);
    };

    connect(model, &QAbstractItemModel::rowsInserted, root, onRowsChanged);
    connect(model, &QAbstractItemModel::rowsRemoved, root, onRowsChanged);
}

// Open selected bookmark when clicked
void BookmarksPluginView::onBookmarkClicked(const QModelIndex &index)
{
    auto selectedRows = m_selectionModel.selectedRows();
    if (selectedRows.length() == 1) {
        if (selectedRows.at(0).row() == index.row()) {
            openBookmark(m_model->getBookmark(m_proxyModel.mapToSource(index)));
        }
    }
}

KTextEditor::View *BookmarksPluginView::openBookmark(const Bookmark &bookmark)
{
    auto view = m_mainWindow->openUrl(bookmark.url);
    view->setCursorPosition(KTextEditor::Cursor(bookmark.lineNumber, 0));
    return view;
}

void BookmarksPluginView::appendDocumentMarks(KTextEditor::Document *document)
{
    auto marks = document->marks();
    for (auto i = marks.cbegin(); i != marks.cend(); ++i) {
        if (i.value()->type == KTextEditor::Document::Bookmark) {
            m_model->addBookmark(document->url(), i.value()->line);
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
            m_model->addBookmark(document->url(), mark.line);
        } else if (action == KTextEditor::Document::MarkChangeAction::MarkRemoved) {
            m_model->removeBookmark(document->url(), mark.line);
        }
    }
}

void BookmarksPluginView::onBackBtnClicked()
{
    QModelIndex index = m_proxyModel.mapToSource(m_treeView->currentIndex());
    int rowCount = m_model->rowCount(index.parent());

    if (rowCount > 0) {
        int prevRow = index.row() - 1;
        if (prevRow < 0) {
            prevRow = rowCount - 1;
        }

        auto prevIndex = m_proxyModel.mapFromSource(m_model->index(prevRow, 0, index.parent()));
        m_treeView->setCurrentIndex(prevIndex);
    }
}

void BookmarksPluginView::onNextBtnClicked()
{
    QModelIndex index = m_proxyModel.mapToSource(m_treeView->currentIndex());
    int rowCount = m_model->rowCount(index.parent());

    if (rowCount > 0) {
        int nextRow = index.row() + 1;
        if (nextRow >= rowCount) {
            nextRow = 0;
        }

        auto nextIndex = m_proxyModel.mapFromSource(m_model->index(nextRow, 0, index.parent()));
        m_treeView->setCurrentIndex(nextIndex);
    }
}

void BookmarksPluginView::onRemoveBtnClicked()
{
    QModelIndex index = m_proxyModel.mapToSource(m_treeView->currentIndex());
    const Bookmark &bookmark = m_model->getBookmark(index);

    auto view = openBookmark(bookmark);
    if (view == nullptr || view->document() == nullptr) {
        return;
    }

    view->document()->removeMark(bookmark.lineNumber, KTextEditor::Document::Bookmark);
}

BookmarksPluginView::~BookmarksPluginView()
{
}

#include "bookmarksplugin.moc"
