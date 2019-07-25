/*  SPDX-License-Identifier: MIT

    Copyright (C) 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
 * Some explanation here on server configuration JSON, pending such ending up
 * in real user level documentation ...
 *
 * The default configuration in JSON format is roughly as follows;
{
    "global":
    {
        "root": null,
    },
    "servers":
    {
        "Python":
        {
            "command": "python3 -m pyls --check-parent-process"
        },
        "C":
        {
            "command": "clangd -log=verbose --background-index"
        },
        "C++":
        {
            "use": "C++"
        }
    }
}
 * (the "command" can be an array or a string, which is then split into array)
 *
 * From the above, the gist is presumably clear.  In addition, each server
 * entry object may also have an "initializationOptions" entry, which is passed
 * along to the server as part of the 'initialize' method.  A clangd-specific
 * HACK^Hfeature uses this to add "compilationDatabasePath".
 *
 * Various stages of override/merge are applied;
 * + user configuration (loaded from file) overrides (internal) default configuration
 * + "lspclient" entry in projectMap overrides the above
 * + the resulting "global" entry is used to supplement (not override) any server entry
 *
 * One server instance is used per (root, servertype) combination.
 * If "root" is not specified, it default to the $HOME directory.  If it is
 * specified as an absolute path, then it used as-is, otherwise it is relative
 * to the projectBase.  For any document, the resulting "root" then determines
 * whether or not a separate instance is needed. If so, the "root" is passed
 * as rootUri/rootPath.
 *
 * In general, it is recommended to leave root unspecified, as it is not that
 * important for a server (your mileage may vary though).  Fewer instances
 * are obviously more efficient, and they also have a 'wider' view than
 * the view of many separate instances.
 */

#include "lspclientservermanager.h"

#include "lspclient_debug.h"

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <KTextEditor/Message>
#include <KLocalizedString>
#include <KTextEditor/MovingInterface>

#include <QTimer>
#include <QEventLoop>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

// local helper;
// recursively merge top json top onto bottom json
static QJsonObject
merge(const QJsonObject & bottom, const QJsonObject & top)
{
    QJsonObject result;
    for (auto item = top.begin(); item != top.end(); item++) {
        const auto & key = item.key();
        if (item.value().isObject())  {
            result.insert(key, merge(bottom.value(key).toObject(), item.value().toObject()));
        } else {
            result.insert(key, item.value());
        }
    }
    // parts only in bottom
    for (auto item = bottom.begin(); item != bottom.end(); item++) {
        if (!result.contains(item.key())) {
            result.insert(item.key(), item.value());
        }
    }
    return result;
}

// helper guard to handle revision (un)lock
struct RevisionGuard
{
    QPointer<KTextEditor::Document> m_doc;
    KTextEditor::MovingInterface *m_movingInterface = nullptr;
    qint64 m_revision = -1;

    RevisionGuard(KTextEditor::Document *doc = nullptr) :
        m_doc(doc),
        m_movingInterface(qobject_cast<KTextEditor::MovingInterface*>(doc)),
        m_revision(-1)
    {
        if (m_movingInterface) {
            m_revision = m_movingInterface->revision();
            m_movingInterface->lockRevision(m_revision);
        }
    }

    // really only need/allow this one (out of 5)
    RevisionGuard(RevisionGuard && other) : RevisionGuard(nullptr)
    {
        std::swap(m_doc, other.m_doc);
        std::swap(m_movingInterface, other.m_movingInterface);
        std::swap(m_revision, other.m_revision);
    }

    void release()
    {
        m_movingInterface = nullptr;
        m_revision = -1;
    }

    ~RevisionGuard()
    {
        // NOTE: hopefully the revision is still valid at this time
        if (m_doc && m_movingInterface && m_revision >= 0) {
            m_movingInterface->unlockRevision(m_revision);
        }
    }
};


class LSPClientRevisionSnapshotImpl : public LSPClientRevisionSnapshot
{
    Q_OBJECT

    typedef LSPClientRevisionSnapshotImpl self_type;

    // std::map has more relaxed constraints on value_type
    std::map<QUrl, RevisionGuard> m_guards;

    Q_SLOT
    void clearRevisions(KTextEditor::Document *doc)
    {
        for (auto &item: m_guards) {
            if (item.second.m_doc == doc) {
                item.second.release();
            }
        }
    }

public:
    void add(KTextEditor::Document *doc)
    {
        Q_ASSERT(doc);

        /* NOTE:
         * The implementation (at least one) of range translation comes down to
         * katetexthistory.cpp.  While there are asserts in place that check
         * for a valid revision parameter, those checks are *only* in assert
         * and will otherwise result in nasty index access.  So it is vital
         * that a valid revision is always given.  However, there is no way
         * to check for a valid revision using the interface only, and documentation
         * refers to (dangerous) effects of clear() and reload() (that are not
         * part of the interface, neither clearly of Document interface).
         * So it then becomes crucial that the following signal covers all
         * cases where a revision might be invalidated.
         * Even if so, it would be preferably being able to check for a valid/known
         * revision at any other time by other means (e.g. interface method).
         */
        auto conn = connect(doc, SIGNAL(aboutToInvalidateMovingInterfaceContent(KTextEditor::Document*)),
            this, SLOT(clearRevisions(KTextEditor::Document*)));
        Q_ASSERT(conn);
#if 0
        // disable until confirmation/clarification on the above
        m_guards.emplace(doc->url(), doc);
#endif
    }

    void find(const QUrl & url, KTextEditor::MovingInterface* & miface, qint64 & revision) const override
    {
        auto it = m_guards.find(url);
        if (it != m_guards.end()) {
            miface = it->second.m_movingInterface;
            revision = it->second.m_revision;
        } else {
            miface = nullptr;
            revision = -1;
        }
    }
};

// helper class to sync document changes to LSP server
class LSPClientServerManagerImpl : public LSPClientServerManager
{
    Q_OBJECT

    typedef LSPClientServerManagerImpl self_type;

    struct DocumentInfo
    {
        QSharedPointer<LSPClientServer> server;
        KTextEditor::MovingInterface *movingInterface;
        QUrl url;
        qint64 version;
        bool open;
        bool modified;
    };

    LSPClientPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    // merged default and user config
    QJsonObject m_serverConfig;
    // root -> (mode -> server)
    QMap<QUrl, QMap<QString, QSharedPointer<LSPClientServer>>> m_servers;
    QHash<KTextEditor::Document*, DocumentInfo> m_docs;

    typedef QVector<QSharedPointer<LSPClientServer>> ServerList;

public:
    LSPClientServerManagerImpl(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin)
        : m_plugin(plugin) , m_mainWindow(mainWin)
    {
        connect(plugin, &LSPClientPlugin::update, this, &self_type::updateServerConfig);
        QTimer::singleShot(100, this, &self_type::updateServerConfig);
    }

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
        for (const auto & el: m_servers) {
            for (const auto & s: el) {
                disconnect(s.get(), nullptr, this, nullptr);
                if (s->state() != LSPClientServer::State::None) {
                    auto handler = [&q, &count, s] () {
                        if (s->state() != LSPClientServer::State::None) {
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
            update(server.get(), false);
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

    virtual LSPClientRevisionSnapshot* snapshot(LSPClientServer *server) override
    {
        auto result = new LSPClientRevisionSnapshotImpl;
        for (auto it = m_docs.begin(); it != m_docs.end(); ++it) {
            if (it->server == server) {
                // sync server to latest revision that will be recorded
                update(it.key(), false);
                result->add(it.key());
            }
        }
        return result;
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
        const auto projectBase = QDir(projectView ? projectView->property("projectBaseDir").toString() : QString());
        const auto& projectMap = projectView ? projectView->property("projectMap").toMap() : QVariantMap();

        auto mode = document->highlightingMode();
        // merge with project specific
        auto projectConfig = QJsonDocument::fromVariant(projectMap).object().value(QStringLiteral("lspclient")).toObject();
        auto serverConfig = merge(m_serverConfig, projectConfig);

        // locate server config
        QJsonValue config;
        QSet<QString> used;
        while (true) {
            qCInfo(LSPCLIENT) << "mode " << mode;
            used << mode;
            config = serverConfig.value(QStringLiteral("servers")).toObject().value(mode);
            if (config.isObject()) {
                const auto & base = config.toObject().value(QStringLiteral("use")).toString();
                // basic cycle detection
                if (!base.isEmpty() && !used.contains(base)) {
                    mode = base;
                    continue;
                }
            }
            break;
        }

        if (!config.isObject())
            return nullptr;

        // merge global settings
        serverConfig = merge(serverConfig.value(QStringLiteral("global")).toObject(), config.toObject());

        QString rootpath;
        auto rootv = serverConfig.value(QStringLiteral("root"));
        if (rootv.isString()) {
            auto sroot = rootv.toString();
            if (QDir::isAbsolutePath(sroot)) {
                rootpath = sroot;
            } else if (!projectBase.isEmpty()) {
                rootpath = QDir(projectBase).absoluteFilePath(sroot);
            }
        }
        if (rootpath.isEmpty()) {
            rootpath = QDir::homePath();
        }

        auto root = QUrl::fromLocalFile(rootpath);
        auto server = m_servers.value(root).value(mode);
        if (!server) {
            QStringList cmdline;
            auto vcmdline = serverConfig.value(QStringLiteral("command"));
            auto scmdline = vcmdline.toString();
            if (!scmdline.isEmpty()) {
                cmdline = scmdline.split(QLatin1Char(' '));
            } else {
                for (const auto& c : vcmdline.toArray()) {
                    cmdline.push_back(c.toString());
                }
            }
            if (cmdline.length() > 0) {
                server.reset(new LSPClientServer(cmdline, root, serverConfig.value(QStringLiteral("initializationOptions"))));
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

    void updateServerConfig()
    {
        // default configuration
        auto makeServerConfig = [] (const QString & cmdline) {
            return QJsonObject {
                { QStringLiteral("command"), cmdline }
            };
        };

        static auto defaultConfig = QJsonObject {
            { QStringLiteral("servers"),
                QJsonObject {
                    { QStringLiteral("Python"),
                        makeServerConfig(QStringLiteral("python3 -m pyls --check-parent-process")) },
                    { QStringLiteral("C"),
                        makeServerConfig(QStringLiteral("clangd -log=%1 --background-index").arg(m_plugin->m_debugMode ? QStringLiteral("verbose") : QStringLiteral("error"))) },
                    { QStringLiteral("C++"),
                        QJsonObject { { QStringLiteral("use"), QStringLiteral("C") } } }
                }
            }
        };

        m_serverConfig = defaultConfig;

        // consider specified configuration
        const auto& configPath = m_plugin->m_configPath.path();
        if (!configPath.isEmpty()) {
            QFile f(configPath);
            if (f.open(QIODevice::ReadOnly)) {
                auto data = f.readAll();
                auto json = QJsonDocument::fromJson(data);
                if (json.isObject()) {
                    m_serverConfig = merge(m_serverConfig, json.object());
                } else {
                    showMessage(i18n("Failed to parse server configuration: %1", configPath),
                        KTextEditor::Message::Error);
                }
            } else {
                showMessage(i18n("Failed to read server configuration: %1", configPath),
                    KTextEditor::Message::Error);
            }
        }

        // we could (but do not) perform restartAll here;
        // for now let's leave that up to user
        // but maybe we do have a server now where not before, so let's signal
        emit serverChanged();
    }

    void trackDocument(KTextEditor::Document *doc, QSharedPointer<LSPClientServer> server)
    {
        auto it = m_docs.find(doc);
        if (it == m_docs.end()) {
            KTextEditor::MovingInterface* miface = qobject_cast<KTextEditor::MovingInterface*>(doc);
            it = m_docs.insert(doc, {server, miface, doc->url(), 0, false, true});
            // track document
            connect(doc, &KTextEditor::Document::documentUrlChanged, this, &self_type::untrack, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::highlightingModeChanged, this, &self_type::untrack, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::aboutToClose, this, &self_type::untrack, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::destroyed, this, &self_type::untrack, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::textChanged, this, &self_type::onTextChanged, Qt::UniqueConnection);
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

    void update(const decltype(m_docs)::iterator & it, bool force)
    {
        auto doc = it.key();
        if (it != m_docs.end() && it->server) {
            if (it->movingInterface) {
                it->version = it->movingInterface->revision();
            } else if (it->modified) {
                ++it->version;
            }
            if (it->open) {
                if (it->modified || force) {
                    (it->server)->didChange(it->url, it->version, doc->text());
                    it->modified = false;
                }
            } else {
                (it->server)->didOpen(it->url, it->version, doc->text());
                it->open = true;
            }
        }
    }

    void update(KTextEditor::Document *doc, bool force) override
    {
        update(m_docs.find(doc), force);
    }

    void update(LSPClientServer * server, bool force)
    {
        for (auto it = m_docs.begin(); it != m_docs.end(); ++it) {
            if (it->server == server) {
                update(it, force);
            }
        }
    }

    void onTextChanged(KTextEditor::Document *doc)
    {
        auto it = m_docs.find(doc);
        if (it != m_docs.end()) {
            it->modified = true;
        }
    }
};

QSharedPointer<LSPClientServerManager>
LSPClientServerManager::new_(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin)
{
    return QSharedPointer<LSPClientServerManager>(new LSPClientServerManagerImpl(plugin, mainWin));
}

#include "lspclientservermanager.moc"
