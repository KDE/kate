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

#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <KTextEditor/Message>
#include <KXMLGUIClient>

#include <QHBoxLayout>
#include <QAction>
#include <QHeaderView>
#include <QTimer>
#include <QSet>


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
    QPointer<QAction> m_hover;
    QPointer<QAction> m_complDocOn;

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
        connect(m_serverManager.get(), &LSPClientServerManager::serverChanged, this, &self_type::updateState);

        m_findDef = actionCollection()->addAction(QStringLiteral("lspclient_find_definition"), this, &self_type::goToDefinition);
        m_findDef->setText(i18n("Go to Definition"));
        m_findDecl = actionCollection()->addAction(QStringLiteral("lspclient_find_declaration"), this, &self_type::goToDeclaration);
        m_findDecl->setText(i18n("Go to Declaration"));
        m_findRef = actionCollection()->addAction(QStringLiteral("lspclient_find_references"), this, &self_type::findReferences);
        m_findRef->setText(i18n("Find References"));
        // perhaps hover suggests to do so on mouse-over,
        // but let's just use a (convenient) action/shortcut for it
        m_hover = actionCollection()->addAction(QStringLiteral("lspclient_hover"), this, &self_type::hover);
        m_hover->setText(i18n("Hover"));

        // completion configuration
        m_complDocOn = actionCollection()->addAction(QStringLiteral("lspclient_completion_doc"), this, &self_type::displayOptionChanged);
        m_complDocOn->setText(i18n("Show selected completion documentation"));
        m_complDocOn->setCheckable(true);

        // popup menu
        auto menu = new KActionMenu(i18n("LSP Client"), this);
        actionCollection()->addAction(QStringLiteral("popup_lspclient"), menu);
        menu->addAction(m_findDef);
        menu->addAction(m_findDecl);
        menu->addAction(m_findRef);
        menu->addAction(m_hover);
        menu->addSeparator();
        menu->addAction(m_complDocOn);

        // sync with plugin settings if updated
        connect(m_plugin, &LSPClientPlugin::update, this, &self_type::configUpdated);

        configUpdated();
        updateState();

        m_mainWindow->guiFactory()->addClient(this);
    }

    ~LSPClientPluginViewImpl()
    {
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
        displayOptionChanged();
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

        m_req_timeout = false;
        QTimer::singleShot(2000, this, [this] { m_req_timeout = true; });
        m_handle.cancel() = req(*server, activeView->document()->url(),
            {cursor.line(), cursor.column()}, this, h);
    }

    void goToLocation(const LocationRequest<DocumentDefinitionReplyHandler> & req)
    {
        auto h = [this] (const QList<LSPLocation> & defs)
        {
            // TODO add another (bottom) view to display definitions
            // in case too late or multiple ones have been found
            // (also adjust timeout then ...)

            if (defs.count()) {
                auto &def = defs.at(0);
                auto &pos = def.range.start;

                KTextEditor::View *activeView = m_mainWindow->activeView();
                // it's not nice to jump to some location if we are too late
                if (!activeView || m_req_timeout || pos.line < 0 || pos.column < 0)
                    return;
                KTextEditor::Document *document = activeView->document();
                KTextEditor::Cursor cdef(pos.line, pos.column);

                if (document && def.uri == document->url()) {
                    activeView->setCursorPosition(cdef);
                } else {
                    KTextEditor::View *view = m_mainWindow->openUrl(def.uri);
                    if (view) {
                        view->setCursorPosition(cdef);
                    }
                }
            }
        };

        positionRequest<DocumentDefinitionReplyHandler>(req, h);
    }

    void goToDefinition()
    {
        goToLocation(&LSPClientServer::documentDefinition);
    }

    void goToDeclaration()
    {
        goToLocation(&LSPClientServer::documentDeclaration);
    }

    void findReferences()
    {
    }

    void hover()
    {
        auto h = [this] (const LSPHover & info)
        {
            KTextEditor::View *view = m_mainWindow->activeView();
            if (!view || !view->document()) return;

            // TODO ?? also indicate range in some way ??
            auto text = info.contents.value.length() ? info.contents.value : i18n("No Hover Info");
            auto msg = new KTextEditor::Message(text, KTextEditor::Message::Information);
            //msg->setWordWrap(true);
            msg->setPosition(KTextEditor::Message::BottomInView);
            msg->setAutoHide(500);
            msg->setView(view);
            view->document()->postMessage(msg);
        };

        positionRequest<DocumentHoverReplyHandler>(&LSPClientServer::documentHover, h);
    }

    void updateState()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        bool defEnabled = false, declEnabled = false, refEnabled = false, hoverEnabled = false;

        if (server) {
            const auto& caps = server->capabilities();
            defEnabled = caps.definitionProvider;
            // FIXME no real official protocol way to detect, so enable anyway
            declEnabled = caps.declarationProvider || true;
            // TODO enable when implemented
            refEnabled = caps.referencesProvider && false;
            hoverEnabled = caps.hoverProvider;
        }

        if (m_findDef)
            m_findDef->setEnabled(defEnabled);
        if (m_findDecl)
            m_findDecl->setEnabled(declEnabled);
        if (m_findRef)
            m_findRef->setEnabled(refEnabled);
        if (m_hover)
            m_hover->setEnabled(hoverEnabled);
        if (m_complDocOn)
            m_complDocOn->setEnabled(server);

        // update completion with relevant server
        m_completion->setServer(server);

        displayOptionChanged();
        updateCompletion(activeView, server.get());
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
