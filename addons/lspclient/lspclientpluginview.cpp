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

        // popup menu
        auto menu = new KActionMenu(i18n("LSP Client"), this);
        actionCollection()->addAction(QStringLiteral("popup_lspclient"), menu);
        menu->addAction(m_findDef);
        menu->addAction(m_findDecl);
        menu->addAction(m_findRef);

        updateState();

        m_mainWindow->guiFactory()->addClient(this);
    }

    ~LSPClientPluginViewImpl()
    {
        m_mainWindow->guiFactory()->removeClient(this);
    }

    void goToDefinition()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        if (!server)
            return;

        auto h = [this] (const QList<LSPDocumentPosition> & defs)
        {
            if (defs.count()) {
                auto &def = defs.at(0);

                KTextEditor::View *activeView = m_mainWindow->activeView();
                // it's not nice to jump to some location if we are too late
                if (!activeView || m_req_timeout)
                    return;
                KTextEditor::Document *document = activeView->document();
                KTextEditor::Cursor cdef(def.position.line, def.position.column);

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
        // TODO add another (bottom) view to display definitions
        // in case too late or multiple ones have been found
        // (also odjust timeout below then ...)

        KTextEditor::Cursor cursor = activeView->cursorPosition();

        m_req_timeout = false;
        QTimer::singleShot(2000, this, [this] { m_req_timeout = true; });
        m_handle.cancel() = server->documentDefinition(activeView->document()->url(),
            {cursor.line(), cursor.column()}, this, h);
    }

    void goToDeclaration()
    {
    }

    void findReferences()
    {
    }

    void updateState()
    {
        KTextEditor::View *activeView = m_mainWindow->activeView();
        auto server = m_serverManager->findServer(activeView);
        bool defEnabled = false, declEnabled = false, refEnabled = false;

        if (server) {
            const auto& caps = server->capabilities();
            defEnabled = caps.definitionProvider;
            // TODO enable when implemented
            declEnabled = caps.declarationProvider && false;
            refEnabled = caps.referencesProvider && false;
        }

        if (m_findDef)
            m_findDef->setEnabled(defEnabled);
        if (m_findDecl)
            m_findDecl->setEnabled(declEnabled);
        if (m_findRef)
            m_findRef->setEnabled(refEnabled);

        // update completion with relevant server
        m_completion->setServer(server);

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
