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
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>

#include <QFile>
#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QSortFilterProxyModel>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

namespace
{

const QString VirtualFileUrlScheme = QStringLiteral("kate");

QUrl getBookmarkUrl(KTextEditor::Document *document)
{
    QUrl url = document->url();
    if (url.isEmpty()) {
        url.setScheme(VirtualFileUrlScheme);
        url.setHost(QStringLiteral());
        url.setPath(document->documentName());
    }

    return url;
}
}

K_PLUGIN_FACTORY_WITH_JSON(BookmarksPluginFactory, "bookmarksplugin.json", registerPlugin<BookmarksPlugin>();)

BookmarksPlugin::~BookmarksPlugin() = default;

BookmarksPlugin::BookmarksPlugin(QObject *parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
    , m_model(parent)
    , m_urls()
{
    // Application instance
    auto app = KTextEditor::Editor::instance()->application();

    // Read meta infos and add bookmarks
    KConfig metaInfos(QStringLiteral("katemetainfos"));
    for (auto groupName : metaInfos.groupList()) {
        KConfigGroup kgroup(&metaInfos, groupName);
        QUrl url(kgroup.readEntry("URL"));
        if (!url.isEmpty() && url.isValid()) {
            // meta infos may reference files that do not exist on disk
            if (url.isLocalFile() && !QFile::exists(url.toLocalFile())) {
                continue;
            }
            const QList<int> marks = kgroup.readEntry("Bookmarks", QList<int>());
            m_model.setBookmarks(url, marks);
        }
    }

    // Register already open documents
    const auto docs = app->documents();
    for (auto document : docs) {
        registerDocument(document);
        syncDocumentBookmarks(document); // If meta infos are disabled, add current document bookmarks
    }

    // Register newly created documents
    connect(app, &KTextEditor::Application::documentCreated, this, &BookmarksPlugin::registerDocument);
}

void BookmarksPlugin::registerDocument(KTextEditor::Document *document)
{
    m_urls[document] = getBookmarkUrl(document);
    connect(document, &KTextEditor::Document::marksChanged, this, &BookmarksPlugin::syncDocumentBookmarks, Qt::UniqueConnection);
    connect(document, &KTextEditor::Document::documentUrlChanged, this, &BookmarksPlugin::onDocumentUrlChanged, Qt::UniqueConnection);
    connect(document, &KTextEditor::Document::aboutToClose, this, &BookmarksPlugin::onDocumentAboutToClose, Qt::UniqueConnection);
    connect(document, &KTextEditor::Document::modifiedOnDisk, this, &BookmarksPlugin::onDocumentModifiedOnDisk, Qt::UniqueConnection);
}

void BookmarksPlugin::onDocumentModifiedOnDisk(KTextEditor::Document *document, bool, KTextEditor::Document::ModifiedOnDiskReason reason)
{
    if (reason == KTextEditor::Document::OnDiskDeleted) {
        QUrl url = getBookmarkUrl(document);
        m_model.setBookmarks(url, {});
    }
}

void BookmarksPlugin::onDocumentAboutToClose(KTextEditor::Document *document)
{
    m_urls.remove(document);
}

void BookmarksPlugin::onDocumentUrlChanged(KTextEditor::Document *document)
{
    QUrl oldUrl = m_urls.value(document);
    QUrl newUrl = getBookmarkUrl(document);
    m_urls[document] = newUrl;

    m_model.setBookmarks(oldUrl, {});
    syncDocumentBookmarks(document);
}

void BookmarksPlugin::syncDocumentBookmarks(KTextEditor::Document *document)
{
    QList<int> lineNumbers;
    auto marks = document->marks();

    for (auto i = marks.cbegin(); i != marks.cend(); ++i) {
        if (i.value()->type == KTextEditor::Document::Bookmark) {
            lineNumbers.append(i.value()->line);
        }
    }

    QUrl url = getBookmarkUrl(document);
    m_model.setBookmarks(url, lineNumbers);
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

    connect(m_treeView, &QTreeView::clicked, this, &BookmarksPluginView::onBookmarkClicked);
    connect(&m_selectionModel, &QItemSelectionModel::selectionChanged, root, [this, removeBtn](const QItemSelection &selected) {
        auto indexes = m_proxyModel.mapSelectionToSource(selected).indexes();
        removeBtn->setEnabled(!indexes.empty());
        if (!indexes.empty()) {
            openBookmark(m_model->getBookmark(indexes.first()));
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
    if (bookmark.url.scheme() == VirtualFileUrlScheme) {
        auto app = KTextEditor::Editor::instance()->application();
        const QList<KTextEditor::Document *> docs = app->documents();
        for (KTextEditor::Document *doc : docs) {
            if (doc && getBookmarkUrl(doc) == bookmark.url) {
                KTextEditor::View *view = m_mainWindow->activateView(doc);
                view->setCursorPosition(KTextEditor::Cursor(bookmark.lineNumber, 0));
                return view;
            }
        }
    } else {
        auto view = m_mainWindow->openUrl(bookmark.url);
        view->setCursorPosition(KTextEditor::Cursor(bookmark.lineNumber, 0));
        return view;
    }

    return nullptr;
}

void BookmarksPluginView::onBackBtnClicked()
{
    QModelIndex index = m_treeView->currentIndex();
    int rowCount = m_proxyModel.rowCount(index.parent());

    if (rowCount > 0) {
        int prevRow = index.row() - 1;
        if (prevRow < 0) {
            prevRow = rowCount - 1;
        }

        auto prevIndex = m_proxyModel.index(prevRow, 0, index.parent());
        m_treeView->setCurrentIndex(prevIndex);
    }
}

void BookmarksPluginView::onNextBtnClicked()
{
    QModelIndex index = m_treeView->currentIndex();
    int rowCount = m_proxyModel.rowCount(index.parent());

    if (rowCount > 0) {
        int nextRow = index.row() + 1;
        if (nextRow >= rowCount) {
            nextRow = 0;
        }

        auto nextIndex = m_proxyModel.index(nextRow, 0, index.parent());
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
