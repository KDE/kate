/***************************************************************************
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

#include "lspclientservermanager.h"

#include "lspclient_debug.h"

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <KTextEditor/Message>
#include <KLocalizedString>

#include <QTimer>
#include <QEventLoop>

// helper class to sync document changes to LSP server
class LSPClientServerManagerImpl : public LSPClientServerManager
{
    Q_OBJECT

    typedef LSPClientServerManagerImpl self_type;

    struct DocumentInfo
    {
        QSharedPointer<LSPClientServer> server;
        QUrl url;
        int version;
        bool open;
    };

    LSPClientPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    // root -> (mode -> server)
    QMap<QUrl, QMap<QString, QSharedPointer<LSPClientServer>>> m_servers;
    QHash<KTextEditor::Document*, DocumentInfo> m_docs;

    typedef QVector<QSharedPointer<LSPClientServer>> ServerList;

public:
    LSPClientServerManagerImpl(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin)
        : m_plugin(plugin) , m_mainWindow(mainWin)
    {}

    ~LSPClientServerManagerImpl()
    {
        // stop everything as we go down
        // several stages;
        // stage 1; request shutdown of all servers (in parallel)
        // (give that some time)
        // stage 2; send TERM
        // stage 3; send KILL

        // stage 1
        QEventLoop q;
        QTimer t;
        connect(&t, &QTimer::timeout, &q, &QEventLoop::quit);

        auto run = [&q, &t] (int ms) {
            t.setSingleShot(true);
            t.start(ms);
            q.exec();
        };

        int count = 0;
        auto const None = LSPClientServer::State::None;
        for (const auto & el: m_servers) {
            for (const auto & s: el) {
                disconnect(s.get(), nullptr, this, nullptr);
                if (s->state() != None) {
                    auto handler = [&q, &count, s] () {
                        if (s->state() != None) {
                            if (--count == 0) {
                                q.quit();
                            }
                        }
                    };
                    connect(s.get(), &LSPClientServer::stateChanged, this, handler);
                    ++count;
                    s->stop(-1, -1);
                }
            }
        }
        run(500);

        // stage 2 and 3
        count = 0;
        for (count = 0; count < 2; ++count) {
            for (const auto & el: m_servers) {
                for (const auto & s: el) {
                    s->stop(count == 0 ? 1 : -1, count == 0 ? -1 : 1);
                }
            }
            run(100);
        }
    }

    QSharedPointer<LSPClientServer>
    findServer(KTextEditor::Document *document, bool updatedoc = true) override
    {
        if (!document || document->url().isEmpty())
            return nullptr;

        auto it = m_docs.find(document);
        auto server = it != m_docs.end() ? it->server : nullptr;
        if (!server) {
            if ((server = _findServer(document)))
                trackDocument(document, server);
        }

        if (server && updatedoc)
            update(document);
        return server;
    }

    QSharedPointer<LSPClientServer>
    findServer(KTextEditor::View *view, bool updatedoc = true) override
    { return view ? findServer(view->document(), updatedoc) : nullptr; }

    // restart a specific server or all servers if server == nullptr
    void restart(LSPClientServer * server) override
    {
        ServerList servers;
        // find entry for server(s) and move out
        for (auto & m : m_servers) {
            for (auto it = m.begin() ; it != m.end(); ) {
                if (!server || it->get() == server) {
                    servers.push_back(*it);
                    it = m.erase(it);
                } else {
                    ++it;
                }
            }
        }
        restart(servers);
    }

private:
    void showMessage(const QString &msg, KTextEditor::Message::MessageType level)
    {
        KTextEditor::View *view = m_mainWindow->activeView();
        if (!view || !view->document()) return;

        auto kmsg = new KTextEditor::Message(xi18nc("@info", "<b>LSP Client:</b> %1", msg), level);
        kmsg->setPosition(KTextEditor::Message::AboveView);
        kmsg->setAutoHide(5000);
        kmsg->setAutoHideMode(KTextEditor::Message::Immediate);
        kmsg->setView(view);
        view->document()->postMessage(kmsg);
    }

    // caller ensures that servers are no longer present in m_servers
    void restart(const ServerList & servers)
    {
        // close docs
        for (const auto & server : servers) {
            // controlling server here, so disable usual state tracking response
            disconnect(server.get(), nullptr, this, nullptr);
            for (auto it = m_docs.begin(); it != m_docs.end(); ) {
                auto &item = it.value();
                if (item.server == server) {
                    // no need to close if server not in proper state
                    if (server->state() != LSPClientServer::State::Running) {
                        item.open = false;
                    }
                    it = _close(it, true);
                } else {
                    ++it;
                }
            }
        }

        // helper captures servers
        auto stopservers = [servers] (int t, int k) {
            for (const auto & server : servers) {
                server->stop(t, k);
            }
        };

        // trigger server shutdown now
        stopservers(-1, -1);

        // initiate delayed stages (TERM and KILL)
        // async, so give a bit more time
        QTimer::singleShot(2 * TIMEOUT_SHUTDOWN, this, [stopservers] () { stopservers(1, -1); });
        QTimer::singleShot(4 * TIMEOUT_SHUTDOWN, this, [stopservers] () { stopservers(-1, 1); });

        // as for the start part
        // trigger interested parties, which will again request a server as needed
        // let's delay this; less chance for server instances to trip over each other
        QTimer::singleShot(6 * TIMEOUT_SHUTDOWN, this, [this] () { emit serverChanged(); });
    }

    void onStateChanged(LSPClientServer *server)
    {
        if (server->state() == LSPClientServer::State::Running) {
            // clear for normal operation
            emit serverChanged();
        } else if (server->state() == LSPClientServer::State::None) {
            // went down
            showMessage(i18n("Server terminated unexpectedly: %1", server->cmdline().join(QLatin1Char(' '))),
                KTextEditor::Message::Warning);
            restart(server);
        }
    }

    QSharedPointer<LSPClientServer>
    _findServer(KTextEditor::Document *document)
    {
        QObject *projectView = m_mainWindow->pluginView(QStringLiteral("kateprojectplugin"));
        const auto projectRoot = projectView ? projectView->property("projectBaseDir").toString() : QString();
        const auto root = QUrl::fromLocalFile(projectRoot);

        auto mode = document->highlightingMode();
        qCInfo(LSPCLIENT) << "mode" << mode;
        auto server = m_servers.value(root).value(mode);
        if (!server) {
            auto servercmd = m_plugin->m_serverCmds.value(mode);
            if (servercmd.length() > 0) {
                auto cmdline = servercmd.split(QStringLiteral(" "));
                server.reset(new LSPClientServer(cmdline, root));
                m_servers[root][mode] = server;
                connect(server.get(), &LSPClientServer::stateChanged,
                    this, &self_type::onStateChanged, Qt::UniqueConnection);
                if (!server->start()) {
                    showMessage(i18n("Failed to start server: %1", cmdline.join(QLatin1Char(' '))),
                        KTextEditor::Message::Error);
                }
            }
        }
        return (server && server->state() == LSPClientServer::State::Running) ? server : nullptr;
    }

    void trackDocument(KTextEditor::Document *doc, QSharedPointer<LSPClientServer> server)
    {
        auto it = m_docs.find(doc);
        if (it == m_docs.end()) {
            it = m_docs.insert(doc, {server, doc->url(), 0, false});
            // track document
            connect(doc, &KTextEditor::Document::documentUrlChanged, this, &self_type::untrack, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::highlightingModeChanged, this, &self_type::untrack, Qt::UniqueConnection);
            // connect(doc, &KTextEditor::Document::modifiedChanged, this, &self_type::close, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::aboutToClose, this, &self_type::untrack, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::destroyed, this, &self_type::untrack, Qt::UniqueConnection);
        } else {
            it->server = server;
        }
    }

    decltype(m_docs)::iterator
    _close(decltype(m_docs)::iterator it, bool remove)
    {
        if (it != m_docs.end()) {
            if (it->open) {
                // release server side (use url as registered with)
                (it->server)->didClose(it->url);
                it->open = false;
            }
            if (remove) {
                disconnect(it.key(), nullptr, this, nullptr);
                it = m_docs.erase(it);
            }
        }
        return it;
    }

    void _close(KTextEditor::Document *doc, bool remove)
    {
        auto it = m_docs.find(doc);
        if (it != m_docs.end()) {
            _close(it, remove);
        }
    }

    void untrack(QObject *doc)
    {
        _close(qobject_cast<KTextEditor::Document*>(doc), true);
        emit serverChanged();
    }

    void close(KTextEditor::Document *doc)
    { _close(doc, false); }

    void update(KTextEditor::Document *doc) override
    {
        auto it = m_docs.find(doc);
        if (it != m_docs.end() && /*doc->isModified() && */it->server) {
            if (it->open) {
                (it->server)->didChange(it->url, ++it->version, doc->text());
            } else {
                (it->server)->didOpen(it->url, ++it->version, doc->text());
                it->open = true;
            }
        }
    }
};

QSharedPointer<LSPClientServerManager>
LSPClientServerManager::new_(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin)
{
    return QSharedPointer<LSPClientServerManager>(new LSPClientServerManagerImpl(plugin, mainWin));
}

#include "lspclientservermanager.moc"
