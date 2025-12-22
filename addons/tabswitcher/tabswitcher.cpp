/* This file is part of the KDE project

   SPDX-FileCopyrightText: 2014-2019 Dominik Haumann <dhaumann@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tabswitcher.h"
#include "tabswitcherfilesmodel.h"
#include "tabswitchertreeview.h"

#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <KActionCollection>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KXMLGUIFactory>

#include <QAction>
#include <QScrollBar>

K_PLUGIN_FACTORY_WITH_JSON(TabSwitcherPluginFactory, "tabswitcherplugin.json", registerPlugin<TabSwitcherPlugin>();)

TabSwitcherPlugin::TabSwitcherPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
{
}

QObject *TabSwitcherPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new TabSwitcherPluginView(this, mainWindow);
}

TabSwitcherPluginView::TabSwitcherPluginView(TabSwitcherPlugin *plugin, KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow)
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
{
    // register this view
    m_plugin->m_views.append(this);

    m_documentsCreatedTimer.setInterval(100);
    m_documentsCreatedTimer.setSingleShot(true);
    m_documentsCreatedTimer.callOnTimeout([this] {
        auto docs = std::move(m_documentsPendingAdd);
        m_documentsPendingAdd = {};
        registerDocuments(docs);
    });

    m_model = new detail::TabswitcherFilesModel(this);
    m_treeView = new TabSwitcherTreeView();
    m_treeView->setModel(m_model);

    KXMLGUIClient::setComponentName(QStringLiteral("tabswitcher"), i18n("Document Switcher"));
    setXMLFile(QStringLiteral("ui.rc"));

    // note: call after m_treeView is created
    setupActions();

    // fill the model
    setupModel();

    // register action in menu
    m_mainWindow->guiFactory()->addClient(this);

    // popup connections
    connect(m_treeView, &TabSwitcherTreeView::pressed, this, &TabSwitcherPluginView::switchToClicked);
    connect(m_treeView, &TabSwitcherTreeView::itemActivated, this, &TabSwitcherPluginView::activateView);

    // track existing documents
    connect(KTextEditor::Editor::instance()->application(), &KTextEditor::Application::documentCreated, this, [this](KTextEditor::Document *doc) {
        m_documentsCreatedTimer.start();
        m_documentsPendingAdd.push_back(doc);
    });
    connect(KTextEditor::Editor::instance()->application(), &KTextEditor::Application::documentWillBeDeleted, this, &TabSwitcherPluginView::unregisterDocument);

    connect(mainWindow, &KTextEditor::MainWindow::widgetAdded, this, &TabSwitcherPluginView::onWidgetCreated);
    connect(mainWindow, &KTextEditor::MainWindow::widgetRemoved, this, &TabSwitcherPluginView::onWidgetRemoved);

    // track lru activation of views to raise the respective documents in the model
    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &TabSwitcherPluginView::raiseView);
}

TabSwitcherPluginView::~TabSwitcherPluginView()
{
    // delete popup widget
    delete m_treeView;

    // unregister action in menu
    m_mainWindow->guiFactory()->removeClient(this);

    // unregister this view
    m_plugin->m_views.removeAll(this);
}

void TabSwitcherPluginView::setupActions()
{
    auto aNext = actionCollection()->addAction(QStringLiteral("view_lru_document_next"));
    aNext->setText(i18n("Last Used Views"));
    aNext->setIcon(QIcon::fromTheme(QStringLiteral("go-next-view-page")));
    KActionCollection::setDefaultShortcut(aNext, Qt::CTRL | Qt::Key_Tab);
    aNext->setWhatsThis(i18n("Opens a list to walk through the list of last used views."));
    aNext->setStatusTip(i18n("Walk through the list of last used views"));
    connect(aNext, &QAction::triggered, this, &TabSwitcherPluginView::walkForward);

    auto aPrev = actionCollection()->addAction(QStringLiteral("view_lru_document_prev"));
    aPrev->setText(i18n("Last Used Views (Reverse)"));
    aPrev->setIcon(QIcon::fromTheme(QStringLiteral("go-previous-view-page")));
    KActionCollection::setDefaultShortcut(aPrev, Qt::CTRL | Qt::SHIFT | Qt::Key_Tab);
    aPrev->setWhatsThis(i18n("Opens a list to walk through the list of last used views in reverse."));
    aPrev->setStatusTip(i18n("Walk through the list of last used views"));
    connect(aPrev, &QAction::triggered, this, &TabSwitcherPluginView::walkBackward);

    auto aClose = actionCollection()->addAction(QStringLiteral("view_lru_document_close"));
    aClose->setText(i18n("Close View"));
    aClose->setShortcutContext(Qt::WidgetShortcut);
    KActionCollection::setDefaultShortcut(aClose, Qt::CTRL | Qt::Key_W);
    aClose->setWhatsThis(i18n("Closes the selected view in the list of last used views."));
    aClose->setStatusTip(i18n("Closes the selected view in the list of last used views."));
    connect(aClose, &QAction::triggered, this, &TabSwitcherPluginView::closeView);

    // make sure action work when the popup has focus
    m_treeView->addAction(aNext);
    m_treeView->addAction(aPrev);
    m_treeView->addAction(aClose);
}

void TabSwitcherPluginView::setupModel()
{
    const auto documents = KTextEditor::Editor::instance()->application()->documents();
    // initial fill of model
    registerDocuments(documents);
}

void TabSwitcherPluginView::registerItem(DocOrWidget docOrWidget)
{
    // insert into hash
    m_documents.insert(docOrWidget);

    // add to model
    m_model->insertDocuments(0, {docOrWidget});
}

void TabSwitcherPluginView::unregisterItem(DocOrWidget docOrWidget)
{
    // remove from hash
    auto it = m_documents.find(docOrWidget);
    if (it == m_documents.end()) {
        // remove from pending
        if (auto doc = docOrWidget.doc()) {
            auto pendingDocIt = std::find(m_documentsPendingAdd.begin(), m_documentsPendingAdd.end(), doc);
            if (pendingDocIt != m_documentsPendingAdd.end()) {
                m_documentsPendingAdd.erase(pendingDocIt);
            }
        }
        return;
    }
    m_documents.erase(it);

    // remove from model
    m_model->removeDocument(docOrWidget);
}

void TabSwitcherPluginView::onWidgetCreated(QWidget *widget)
{
    registerItem(widget);
}

void TabSwitcherPluginView::onWidgetRemoved(QWidget *widget)
{
    unregisterItem(widget);
}

void TabSwitcherPluginView::registerDocuments(const QList<KTextEditor::Document *> &documents)
{
    if (documents.isEmpty()) {
        return;
    }
    m_documents.insert(documents.begin(), documents.end());
    QList<DocOrWidget> docs;
    docs.reserve(documents.size());
    for (auto d : documents) {
        connect(d, &KTextEditor::Document::documentNameChanged, this, &TabSwitcherPluginView::updateDocumentName);
        docs.push_back(DocOrWidget(d));
    }

    m_model->insertDocuments(0, docs);
}

void TabSwitcherPluginView::unregisterDocument(KTextEditor::Document *document)
{
    unregisterItem(document);
    // disconnect documentNameChanged() signal
    disconnect(document, nullptr, this, nullptr);
}

void TabSwitcherPluginView::updateDocumentName(KTextEditor::Document *document)
{
    if (!m_documents.contains(document)) {
        return;
    }

    // update all items, since a document URL change means we have to recalculate
    // common prefix path of all items.
    m_model->updateItems();
}

void TabSwitcherPluginView::raiseView(KTextEditor::View *view)
{
    auto activeWidget = [this, view]() -> DocOrWidget {
        if (view && view->document()) {
            return view->document();
        }
        return m_mainWindow->activeWidget();
    }();

    if (activeWidget.isNull() || m_documents.find(activeWidget) == m_documents.end()) {
        return;
    }

    m_model->raiseDocument(activeWidget);
}

void TabSwitcherPluginView::walk(const int from, const int to)
{
    if (m_model->rowCount() <= 1) {
        return;
    }

    QModelIndex index;
    const int step = from < to ? 1 : -1;
    if (!m_treeView->isVisible()) {
        updateViewGeometry();
        index = m_model->index(from + step, 0);
        if (!index.isValid()) {
            index = m_model->index(0, 0);
        }
        m_treeView->show();
        m_treeView->setFocus();
    } else {
        int newRow = m_treeView->selectionModel()->currentIndex().row() + step;
        if (newRow == to + step) {
            newRow = from;
        }
        index = m_model->index(newRow, 0);
    }

    m_treeView->selectionModel()->select(index, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
    m_treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

void TabSwitcherPluginView::walkForward()
{
    walk(0, m_model->rowCount() - 1);
}

void TabSwitcherPluginView::walkBackward()
{
    walk(m_model->rowCount() - 1, 0);
}

void TabSwitcherPluginView::updateViewGeometry()
{
    QWidget *window = m_mainWindow->window();
    const QSize centralSize = window->size();

    // Maximum size of the view is 3/4th of the central widget (the editor area)
    // so the view does not overlap the mainwindow since that looks awkward.
    const QSize viewMaxSize(centralSize.width() * 3 / 4, centralSize.height() * 3 / 4);

    // The actual view size should be as big as the columns/rows need it, but
    // smaller than the max-size. This means the view will get quite high with
    // many open files but I think that's ok. Otherwise one can easily tweak the
    // max size to be only 1/2th of the central widget size
    const int rowHeight = m_treeView->sizeHintForRow(0);
    const int frameWidth = m_treeView->frameWidth();
    // const QSize viewSize(std::min(m_treeView->sizeHintForColumn(0) + 2 * frameWidth + m_treeView->verticalScrollBar()->width(), viewMaxSize.width()), // ORIG
    // line, sizeHintForColumn was QListView but is protected for QTreeView so we introduced sizeHintWidth()
    const QSize viewSize(std::min(m_treeView->sizeHintWidth() + (2 * frameWidth) + m_treeView->verticalScrollBar()->width(), viewMaxSize.width()),
                         std::min(std::max((rowHeight * m_model->rowCount()) + (2 * frameWidth), rowHeight * 6), viewMaxSize.height()));

    // Position should be central over the editor area, so map to global from
    // parent of central widget since the view is positioned in global coords
    const QPoint centralWidgetPos = window->parent() ? window->mapToGlobal(window->pos()) : window->pos();
    const int xPos = std::max(0, centralWidgetPos.x() + ((centralSize.width() - viewSize.width()) / 2));
    const int yPos = std::max(0, centralWidgetPos.y() + ((centralSize.height() - viewSize.height()) / 2));

    m_treeView->setFixedSize(viewSize);
    m_treeView->move(xPos, yPos);
}

void TabSwitcherPluginView::switchToClicked(const QModelIndex &index)
{
    m_treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    activateView(index);
}

void TabSwitcherPluginView::activateView(const QModelIndex &index)
{
    Q_UNUSED(index)

    // guard against empty selection
    if (m_treeView->selectionModel()->selectedRows().isEmpty()) {
        return;
    }

    const int row = m_treeView->selectionModel()->selectedRows().first().row();

    auto doc = m_model->item(row);
    if (doc.doc()) {
        m_mainWindow->activateView(doc.doc());
    } else if (doc.widget()) {
        m_mainWindow->activateWidget(doc.widget());
    }

    m_treeView->hide();
}

void TabSwitcherPluginView::closeView()
{
    if (m_treeView->selectionModel()->selectedRows().isEmpty()) {
        return;
    }

    const int row = m_treeView->selectionModel()->selectedRows().first().row();
    auto doc = m_model->item(row);
    if (doc.doc()) {
        KTextEditor::Editor::instance()->application()->closeDocument(doc.doc());
    } else if (doc.widget()) {
        m_mainWindow->removeWidget(doc.widget());
    }
}

// required for TabSwitcherPluginFactory vtable
#include "tabswitcher.moc"
