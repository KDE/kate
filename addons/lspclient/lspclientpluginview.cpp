/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
 *   Copyright (C) 2019 by Mark Nauwelaerts <mark.nauwelaerts@gmail.com>   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "lspclientpluginview.h"
#include "lspclientsymbolview.h"
#include "lspclientplugin.h"
#include "lspclientservermanager.h"
#include "lspclientcompletion.h"

#include "lspclient_debug.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KLocalizedString>
#include <KStandardAction>
#include <KXMLGUIFactory>
#include <KAcceleratorManager>

#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <KTextEditor/Message>
#include <KTextEditor/MovingInterface>
#include <KXMLGUIClient>

#include <ktexteditor/markinterface.h>
#include <ktexteditor/movinginterface.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/configinterface.h>

#include <QKeyEvent>
#include <QHBoxLayout>
#include <QAction>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTimer>
#include <QSet>

namespace RangeData
{

enum {
    FileUrlRole = Qt::UserRole,
    StartLineRole,
    StartColumnRole,
    EndLineRole,
    EndColumnRole,
    KindRole
};

KTextEditor::MarkInterface::MarkTypes markType = KTextEditor::MarkInterface::markType31;

}

class LSPClientPluginViewImpl : public QObject, public KXMLGUIClient
{
    Q_OBJECT

    typedef LSPClientPluginViewImpl self_type;

    LSPClientPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    QSharedPointer<LSPClientServerManager> m_serverManager;
    QScopedPointer<LSPClientCompletion> m_completion;
    QScopedPointer<QObject> m_symbolView;

    QPointer<QAction> m_findDef;
    QPointer<QAction> m_findDecl;
    QPointer<QAction> m_findRef;
    QPointer<QAction> m_highlight;
    QPointer<QAction> m_hover;
    QPointer<QAction> m_complDocOn;
    QPointer<QAction> m_refDeclaration;
    QPointer<QAction> m_restartServer;
    QPointer<QAction> m_restartAll;

    // toolview
    QScopedPointer<QWidget> m_toolView;
    QPointer<QTabWidget> m_tabWidget;
    // applied ranges
    QMultiHash<KTextEditor::Document*, KTextEditor::MovingRange*> m_ranges;
    // tree is either added to tabwidget or owned here
    QScopedPointer<QTreeWidget> m_ownedTree;
    // in either case, the tree that directs applying marks/ranges
    QPointer<QTreeWidget> m_markTree;

    // views on which completions have been registered
    QSet<KTextEditor::View *> m_completionViews;
    // outstanding request
    LSPClientServer::RequestHandle m_handle;
    // timeout on request
    bool m_req_timeout = false;

public:
    LSPClientPluginViewImpl(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin)
        : QObject(mainWin), m_plugin(plugin), m_mainWindow(mainWin),
          m_serverManager(LSPClientServerManager::new_(plugin, mainWin)),
          m_completion(LSPClientCompletion::new_(m_serverManager)),
          m_symbolView(LSPClientSymbolView::new_(plugin, mainWin, m_serverManager))
    {
        KXMLGUIClient::setComponentName(QStringLiteral("lspclient"), i18n("LSP Client"));
        setXMLFile(QStringLiteral("ui.rc"));

        connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &self_type::updateState);
        connect(m_mainWindow, &KTextEditor::MainWindow::unhandledShortcutOverride, this, &self_type::handleEsc);
        connect(m_serverManager.get(), &LSPClientServerManager::serverChanged, this, &self_type::updateState);

        m_findDef = actionCollection()->addAction(QStringLiteral("lspclient_find_definition"), this, &self_type::goToDefinition);
        m_findDef->setText(i18n("Go to Definition"));
        m_findDecl = actionCollection()->addAction(QStringLiteral("lspclient_find_declaration"), this, &self_type::goToDeclaration);
        m_findDecl->setText(i18n("Go to Declaration"));
        m_findRef = actionCollection()->addAction(QStringLiteral("lspclient_find_references"), this, &self_type::findReferences);
        m_findRef->setText(i18n("Find References"));
        m_highlight = actionCollection()->addAction(QStringLiteral("lspclient_highlight"), this, &self_type::highlight);
        m_highlight->setText(i18n("Highlight"));
        // perhaps hover suggests to do so on mouse-over,
        // but let's just use a (convenient) action/shortcut for it
        m_hover = actionCollection()->addAction(QStringLiteral("lspclient_hover"), this, &self_type::hover);
        m_hover->setText(i18n("Hover"));

        // general options
        m_complDocOn = actionCollection()->addAction(QStringLiteral("lspclient_completion_doc"), this, &self_type::displayOptionChanged);
        m_complDocOn->setText(i18n("Show selected completion documentation"));
        m_complDocOn->setCheckable(true);
        m_refDeclaration = actionCollection()->addAction(QStringLiteral("lspclient_references_declaration"), this, &self_type::displayOptionChanged);
        m_refDeclaration->setText(i18n("Include declaration in references"));
        m_refDeclaration->setCheckable(true);

        // server control
        m_restartServer = actionCollection()->addAction(QStringLiteral("lspclient_restart_server"), this, &self_type::restartCurrent);
        m_restartServer->setText(i18n("Restart LSP Server"));
        m_restartAll = actionCollection()->addAction(QStringLiteral("lspclient_restart_all"), this, &self_type::restartAll);
        m_restartAll->setText(i18n("Restart All LSP Servers"));

        // popup menu
        auto menu = new KActionMenu(i18n("LSP Client"), this);
        actionCollection()->addAction(QStringLiteral("popup_lspclient"), menu);
        menu->addAction(m_findDef);
        menu->addAction(m_findDecl);
        menu->addAction(m_findRef);
        menu->addAction(m_highlight);
        menu->addAction(m_hover);
        menu->addSeparator();
        menu->addAction(m_complDocOn);
        menu->addAction(m_refDeclaration);
        menu->addSeparator();
        menu->addAction(m_restartServer);
        menu->addAction(m_restartAll);

        // sync with plugin settings if updated
        connect(m_plugin, &LSPClientPlugin::update, this, &self_type::configUpdated);

        // toolview
        m_toolView.reset(mainWin->createToolView(plugin, QStringLiteral("kate_lspclient"),
                                                 KTextEditor::MainWindow::Bottom,
                                                 QIcon::fromTheme(QStringLiteral("application-x-ms-dos-executable")),
                                                 i18n("LSP Client")));
        m_tabWidget = new QTabWidget(m_toolView.get());
        m_tabWidget->setTabsClosable(true);
        KAcceleratorManager::setNoAccel(m_tabWidget);
        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &self_type::tabCloseRequested);

        configUpdated();
        updateState();

        m_mainWindow->guiFactory()->addClient(this);
    }

    ~LSPClientPluginViewImpl()
    {
        clearAllMarks();
        m_mainWindow->guiFactory()->removeClient(this);
    }

    void displayOptionChanged()
    {
        if (m_complDocOn)
            m_completion->setSelectedDocumentation(m_complDocOn->isChecked());
    }

    void configUpdated()
    {
        if (m_complDocOn)
            m_complDocOn->setChecked(m_plugin->m_complDoc);
        if (m_refDeclaration)
            m_refDeclaration->setChecked(m_plugin->m_refDeclaration);
        displayOptionChanged();
    }

    void restartCurrent()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        if (server)
            m_serverManager->restart(server.get());
    }

    void restartAll()
    {
        m_serverManager->restart(nullptr);
    }

    Q_SLOT void clearMarks(KTextEditor::Document *doc)
    {
        KTextEditor::MarkInterface* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
        if (iface) {
            const QHash<int, KTextEditor::Mark*> marks = iface->marks();
            QHashIterator<int, KTextEditor::Mark*> i(marks);
            while (i.hasNext()) {
                i.next();
                if (i.value()->type & RangeData::markType) {
                    iface->removeMark(i.value()->line, RangeData::markType);
                }
            }
        }

        for (auto it = m_ranges.find(doc); it != m_ranges.end() && it.key() == doc;) {
            delete it.value();
            it = m_ranges.erase(it);
        }
    }

    void clearAllMarks()
    {
        while (!m_ranges.empty()) {
            clearMarks(m_ranges.begin().key());
        }
        // no longer add any again
        m_ownedTree.reset();
        m_markTree.clear();
    }

    void addMarks(KTextEditor::Document *doc, QTreeWidgetItem *item)
    {
        KTextEditor::MovingInterface* miface = qobject_cast<KTextEditor::MovingInterface*>(doc);
        KTextEditor::MarkInterface* iface = qobject_cast<KTextEditor::MarkInterface*>(doc);
        KTextEditor::View* activeView = m_mainWindow->activeView();
        KTextEditor::ConfigInterface* ciface = qobject_cast<KTextEditor::ConfigInterface*>(activeView);

        if (!miface || !iface)
            return;

        auto url = item->data(0, RangeData::FileUrlRole).toUrl();
        if (url != doc->url())
            return;

        int line = item->data(0, RangeData::StartLineRole).toInt();
        int column = item->data(0, RangeData::StartColumnRole).toInt();
        int endLine = item->data(0, RangeData::EndLineRole).toInt();
        int endColumn = item->data(0, RangeData::EndColumnRole).toInt();
        LSPDocumentHighlightKind kind = (LSPDocumentHighlightKind) item->data(0, RangeData::KindRole).toInt();

        KTextEditor::Range range(line, column, endLine, endColumn);
        KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute());

        QColor rangeColor;
        switch (kind) {
            case LSPDocumentHighlightKind::Text:
                // well, it's a bit like searching for something, so re-use that color
                rangeColor = Qt::yellow;
                if (ciface) {
                    rangeColor = ciface->configValue(QStringLiteral("search-highlight-color")).value<QColor>();
                }
                attr->setBackground(Qt::yellow);
                break;
                // FIXME are there any symbolic/configurable ways to pick these colors?
            case LSPDocumentHighlightKind::Read:
                rangeColor = Qt::green;
                break;
            case LSPDocumentHighlightKind::Write:
                rangeColor = Qt::red;
                break;
        };
        attr->setBackground(rangeColor);
        if (activeView) {
            attr->setForeground(activeView->defaultStyleAttribute(KTextEditor::dsNormal)->foreground().color());
        }

        // highlight the range
        KTextEditor::MovingRange* mr = miface->newMovingRange(range);
        mr->setAttribute(attr);
        mr->setZDepth(-90000.0); // Set the z-depth to slightly worse than the selection
        mr->setAttributeOnlyForViews(true);
        m_ranges.insert(doc, mr);

        // add match mark for range
        iface->setMarkDescription(RangeData::markType, i18n("RangeHighLight"));
        iface->setMarkPixmap(RangeData::markType, QIcon().pixmap(0,0));
        iface->addMark(line, RangeData::markType);

        // ensure runtime match
        auto conn = connect(doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
            this, SLOT(clearMarks(KTextEditor::Document*)), Qt::UniqueConnection);
        Q_ASSERT(conn);
        conn = connect(doc, SIGNAL(aboutToDeleteMovingInterfaceContent(KTextEditor::Document*)),
            this, SLOT(clearMarks(KTextEditor::Document*)), Qt::UniqueConnection);
        Q_ASSERT(conn);
    }

    void addMarks(KTextEditor::Document *doc, QTreeWidget *tree)
    {
        // check if already added
        if (m_ranges.contains(doc))
            return;

        QTreeWidgetItemIterator it(tree, QTreeWidgetItemIterator::All);
        while (*it) {
            addMarks(doc, *it);
             ++it;
         }
    }

    void goToDocumentLocation(const QUrl & uri, int line, int column)
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        if (!activeView || uri.isEmpty() || line < 0 || column < 0)
            return;

        KTextEditor::Document *document = activeView->document();
        KTextEditor::Cursor cdef(line, column);

        if (document && uri == document->url()) {
            activeView->setCursorPosition(cdef);
        } else {
            KTextEditor::View *view = m_mainWindow->openUrl(uri);
            if (view) {
                view->setCursorPosition(cdef);
            }
        }
    }

    void goToItemLocation(QTreeWidgetItem *it)
    {
        if (it) {
            auto url = it->data(0, RangeData::FileUrlRole).toUrl();
            auto line = it->data(0, RangeData::StartLineRole).toInt();
            auto column = it->data(0, RangeData::StartColumnRole).toInt();
            goToDocumentLocation(url, line, column);
        }
    }

    void tabCloseRequested(int index)
    {
        delete m_tabWidget->widget(index);
    }

    // local helper to overcome some differences in LSP types
    struct RangeItem
    {
        QUrl uri;
        LSPRange range;
        LSPDocumentHighlightKind kind;
    };

    static
    bool compareRangeItem(const RangeItem & a, const RangeItem & b)
    { return (a.uri < b.uri) || ((a.uri == b.uri) && a.range < b.range); }

    void makeTree(const QVector<RangeItem> & locations)
    {
        // TODO improve tree display to be more useful and look like
        // search plugin results ...
        // This means we will need to go and get line content;
        // do so at once for openened documents
        // but do so only upon expansion for unopened documents

        QStringList titles;
        // let's not bother translators yet ...
        // titles << i18nc("@title:column", "Location");
        titles << QStringLiteral("Location");

        auto treeWidget = new QTreeWidget();
        treeWidget->setHeaderLabels(titles);
        treeWidget->setRootIsDecorated(0);
        treeWidget->setFocusPolicy(Qt::NoFocus);
        treeWidget->setLayoutDirection(Qt::LeftToRight);
        treeWidget->setColumnCount(1);
        treeWidget->setSortingEnabled(false);
        for (const auto & loc: locations) {
            auto item = new QTreeWidgetItem(treeWidget);
            item->setText(0, QStringLiteral("%1 %2").arg(loc.uri.path()).arg(loc.range.start().line() + 1, 6));
            item->setData(0, RangeData::FileUrlRole, QVariant(loc.uri));
            item->setData(0, RangeData::StartLineRole, loc.range.start().line());
            item->setData(0, RangeData::StartColumnRole, loc.range.start().column());
            item->setData(0, RangeData::EndLineRole, loc.range.end().line());
            item->setData(0, RangeData::EndColumnRole, loc.range.end().column());
            item->setData(0, RangeData::KindRole, (int) loc.kind);
        }
        treeWidget->sortItems(0, Qt::AscendingOrder);
        treeWidget->setSortingEnabled(true);

        m_ownedTree.reset(treeWidget);
        m_markTree = treeWidget;
    }

    void showTree(const QString & title)
    {
        // transfer widget from owned to tabwidget
        auto treeWidget = m_ownedTree.take();
        int index = m_tabWidget->addTab(treeWidget, title);
        connect(treeWidget, &QTreeWidget::itemClicked, this, &self_type::goToItemLocation);

        // activate the resulting tab
        m_tabWidget->setCurrentIndex(index);
        m_mainWindow->showToolView(m_toolView.get());
    }

    void showMessage(const QString & text, KTextEditor::Message::MessageType level)
    {
        KTextEditor::View *view = m_mainWindow->activeView();
        if (!view || !view->document()) return;

        auto kmsg = new KTextEditor::Message(text, level);
        kmsg->setPosition(KTextEditor::Message::BottomInView);
        kmsg->setAutoHide(500);
        kmsg->setView(view);
        view->document()->postMessage(kmsg);
    }

    void handleEsc(QEvent *e)
    {
        if (!m_mainWindow) return;

        QKeyEvent *k = static_cast<QKeyEvent *>(e);
        if (k->key() == Qt::Key_Escape && k->modifiers() == Qt::NoModifier) {
            if (!m_ranges.empty()) {
                clearAllMarks();
            } else if (m_toolView->isVisible()) {
                m_mainWindow->hideToolView(m_toolView.get());
            }
        }
    }

    template<typename Handler>
    using LocationRequest = std::function<LSPClientServer::RequestHandle(LSPClientServer &,
        const QUrl & document, const LSPPosition & pos,
        const QObject *context, const Handler & h)>;

    template<typename Handler>
    void positionRequest(const LocationRequest<Handler> & req, const Handler & h)
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        if (!server)
            return;

        KTextEditor::Cursor cursor = activeView->cursorPosition();

        clearAllMarks();
        m_req_timeout = false;
        QTimer::singleShot(1000, this, [this] { m_req_timeout = true; });
        m_handle.cancel() = req(*server, activeView->document()->url(),
            {cursor.line(), cursor.column()}, this, h);
    }

    QString currentWord()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        if (activeView) {
            KTextEditor::Cursor cursor = activeView->cursorPosition();
            return activeView->document()->wordAt(cursor);
        } else {
            return QString();
        }
    }

    // some template and function type trickery here, but at least that buck stops here then ...
    template<typename ReplyEntryType, bool doshow = true, typename HandlerType = ReplyHandler<QList<ReplyEntryType>>>
    void processLocations(const QString & title,
        const typename utils::identity<LocationRequest<HandlerType>>::type & req, bool onlyshow,
        const std::function<RangeItem(const ReplyEntryType &)> & itemConverter)
    {
        auto h = [this, title, onlyshow, itemConverter] (const QList<ReplyEntryType> & defs)
        {
            if (defs.count() == 0) {
                showMessage(i18n("No results"), KTextEditor::Message::Information);
            } else {
                // convert to helper type
                QVector<RangeItem> ranges;
                ranges.reserve(defs.size());
                for (const auto & def: defs) {
                    ranges.push_back(itemConverter(def));
                }
                // ... so we can sort it also
                std::stable_sort(ranges.begin(), ranges.end(), compareRangeItem);
                makeTree(ranges);

                if (defs.count() > 1 || onlyshow) {
                    showTree(title);
                }
                // it's not nice to jump to some location if we are too late
                if (!m_req_timeout && !onlyshow) {
                    // assuming here that the first location is the best one
                    const auto &item = itemConverter(defs.at(0));
                    const auto &pos = item.range.start();
                    goToDocumentLocation(item.uri, pos.line(), pos.column());
                }
                // update marks
                updateState();
            }
        };

        positionRequest<HandlerType>(req, h);
    }

    static RangeItem
    locationToRangeItem(const LSPLocation & loc)
    { return {loc.uri, loc.range, LSPDocumentHighlightKind::Text}; }

    void goToDefinition()
    {
        auto title = i18nc("@title:tab", "Definition: %1", currentWord());
        processLocations<LSPLocation>(title, &LSPClientServer::documentDefinition, false, &self_type::locationToRangeItem);
    }

    void goToDeclaration()
    {
        auto title = i18nc("@title:tab", "Declaration: %1", currentWord());
        processLocations<LSPLocation>(title, &LSPClientServer::documentDeclaration, false, &self_type::locationToRangeItem);
    }

    void findReferences()
    {
        auto title = i18nc("@title:tab", "References: %1", currentWord());
        bool decl = m_refDeclaration->isChecked();
        auto req = [decl] (LSPClientServer & server, const QUrl & document, const LSPPosition & pos,
                    const QObject *context, const DocumentDefinitionReplyHandler & h)
        { return server.documentReferences(document, pos, decl, context, h); };

        processLocations<LSPLocation>(title, req, true, &self_type::locationToRangeItem);
    }

    void highlight()
    {
        // determine current url to capture and use later on
        QUrl url;
        const KTextEditor::View* viewForRequest(m_mainWindow->activeView());
        if (viewForRequest && viewForRequest->document()) {
            url = viewForRequest->document()->url();
        }

        auto title = i18nc("@title:tab", "Highlight: %1", currentWord());
        auto converter = [url] (const LSPDocumentHighlight & hl)
        { return RangeItem {url, hl.range, hl.kind}; };

        processLocations<LSPDocumentHighlight, false>(title, &LSPClientServer::documentHighlight, true, converter);
    }

    void hover()
    {
        auto h = [this] (const LSPHover & info)
        {
            // TODO ?? also indicate range in some way ??
            auto text = info.contents.value.length() ? info.contents.value : i18n("No Hover Info");
            showMessage(text, KTextEditor::Message::Information);
        };

        positionRequest<DocumentHoverReplyHandler>(&LSPClientServer::documentHover, h);
    }

    void updateState()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        bool defEnabled = false, declEnabled = false, refEnabled = false, hoverEnabled = false, highlightEnabled = false;

        if (server) {
            const auto& caps = server->capabilities();
            defEnabled = caps.definitionProvider;
            // FIXME no real official protocol way to detect, so enable anyway
            declEnabled = caps.declarationProvider || true;
            refEnabled = caps.referencesProvider;
            hoverEnabled = caps.hoverProvider;
            highlightEnabled = caps.documentHighlightProvider;
        }

        if (m_findDef)
            m_findDef->setEnabled(defEnabled);
        if (m_findDecl)
            m_findDecl->setEnabled(declEnabled);
        if (m_findRef)
            m_findRef->setEnabled(refEnabled);
        if (m_highlight)
            m_highlight->setEnabled(highlightEnabled);
        if (m_hover)
            m_hover->setEnabled(hoverEnabled);
        if (m_complDocOn)
            m_complDocOn->setEnabled(server);
        if (m_restartServer)
            m_restartServer->setEnabled(server);

        // update completion with relevant server
        m_completion->setServer(server);

        displayOptionChanged();
        updateCompletion(activeView, server.get());

        // update marks if applicable
        if (m_markTree && activeView)
            addMarks(activeView->document(), m_markTree);
    }

    void viewDestroyed(QObject *view)
    {
        m_completionViews.remove(static_cast<KTextEditor::View *>(view));
    }

    void updateCompletion(KTextEditor::View *view, LSPClientServer * server)
    {
        bool registered = m_completionViews.contains(view);

        KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);

        if (!cci) {
            return;
        }

        if (!registered && server && server->capabilities().completionProvider.provider) {
            qCInfo(LSPCLIENT) << "registering cci";
            cci->registerCompletionModel(m_completion.get());
            m_completionViews.insert(view);

            connect(view, &KTextEditor::View::destroyed, this,
                &self_type::viewDestroyed,
                Qt::UniqueConnection);
        }

        if (registered && !server) {
            qCInfo(LSPCLIENT) << "unregistering cci";
            cci->unregisterCompletionModel(m_completion.get());
            m_completionViews.remove(view);
        }
    }
};

QObject*
LSPClientPluginView::new_(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin)
{
    return new LSPClientPluginViewImpl(plugin, mainWin);
}

#include "lspclientpluginview.moc"
