/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

/* see plugins.docbook lspclient-configuration
 * for client configuration documentation
 */

#include "lspclientservermanager.h"

#include "hostprocess.h"
#include "ktexteditor_utils.h"
#include "lspclient_debug.h"

#include <KLocalizedString>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QThread>
#include <QTime>
#include <QTimer>

#include <json_utils.h>

// sadly no common header for plugins to include this from
// unless we do come up with such one
typedef QMap<QString, QString> QStringMap;
Q_DECLARE_METATYPE(QStringMap)

// helper to find a proper root dir for the given document & file name/pattern that indicates the root dir
static QString findRootForDocument(KTextEditor::Document *document, const QStringList &rootIndicationFileNames, const QStringList &rootIndicationFilePatterns)
{
    // skip search if nothing there to look at
    if (rootIndicationFileNames.isEmpty() && rootIndicationFilePatterns.isEmpty()) {
        return QString();
    }

    // search only feasible if document is local file
    if (!document->url().isLocalFile()) {
        return QString();
    }

    // search root upwards
    QDir dir(QFileInfo(document->url().toLocalFile()).absolutePath());
    QSet<QString> seenDirectories;
    while (!seenDirectories.contains(dir.absolutePath())) {
        // update guard
        seenDirectories.insert(dir.absolutePath());

        // the file that indicates the root dir is there => all fine
        for (const auto &fileName : rootIndicationFileNames) {
            if (dir.exists(fileName)) {
                return dir.absolutePath();
            }
        }

        // look for matching file patterns, if any
        if (!rootIndicationFilePatterns.isEmpty()) {
            dir.setNameFilters(rootIndicationFilePatterns);
            if (!dir.entryList().isEmpty()) {
                return dir.absolutePath();
            }
        }

        // else: cd up, if possible or abort
        if (!dir.cdUp()) {
            break;
        }
    }

    // no root found, bad luck
    return QString();
}

static QStringList indicationDataToStringList(const QJsonValue &indicationData)
{
    if (indicationData.isArray()) {
        QStringList indications;
        for (auto indication : indicationData.toArray()) {
            if (indication.isString()) {
                indications << indication.toString();
            }
        }

        return indications;
    }

    return {};
}

static LSPClientServer::TriggerCharactersOverride parseTriggerOverride(const QJsonValue &json)
{
    LSPClientServer::TriggerCharactersOverride adjust;
    if (json.isObject()) {
        auto ob = json.toObject();
        for (const auto &c : ob.value(QStringLiteral("exclude")).toString()) {
            adjust.exclude.push_back(c);
        }
        for (const auto &c : ob.value(QStringLiteral("include")).toString()) {
            adjust.include.push_back(c);
        }
    }
    return adjust;
}

#include <memory>

// helper guard to handle revision (un)lock
struct RevisionGuard {
    QPointer<KTextEditor::Document> m_doc;
    qint64 m_revision = -1;

    RevisionGuard(KTextEditor::Document *doc = nullptr)
        : m_doc(doc)
    {
        m_revision = doc->revision();
        doc->lockRevision(m_revision);
    }

    // really only need/allow this one (out of 5)
    RevisionGuard(RevisionGuard &&other)
        : RevisionGuard(nullptr)
    {
        std::swap(m_doc, other.m_doc);
        std::swap(m_revision, other.m_revision);
    }

    void release()
    {
        m_revision = -1;
    }

    ~RevisionGuard()
    {
        // NOTE: hopefully the revision is still valid at this time
        if (m_doc && m_revision >= 0) {
            m_doc->unlockRevision(m_revision);
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
        for (auto &item : m_guards) {
            if (item.second.m_doc == doc) {
                item.second.release();
            }
        }
    }

public:
    void add(KTextEditor::Document *doc)
    {
        Q_ASSERT(doc);

        // make sure revision is cleared when needed and no longer used (to unlock or otherwise)
        // see e.g. implementation in katetexthistory.cpp and assert's in place there
        connect(doc, &KTextEditor::Document::aboutToInvalidateMovingInterfaceContent, this, &self_type::clearRevisions);
        connect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &self_type::clearRevisions);
        m_guards.emplace(doc->url(), doc);
    }

    void find(const QUrl &url, KTextEditor::Document *&doc, qint64 &revision) const override
    {
        auto it = m_guards.find(url);
        if (it != m_guards.end()) {
            doc = it->second.m_doc;
            revision = it->second.m_revision;
        } else {
            doc = nullptr;
            revision = -1;
        }
    }
};

static const QString PROJECT_PLUGIN{QStringLiteral("kateprojectplugin")};

// helper class to sync document changes to LSP server
class LSPClientServerManagerImpl : public LSPClientServerManager
{
    Q_OBJECT

    typedef LSPClientServerManagerImpl self_type;

    struct ServerInfo {
        std::shared_ptr<LSPClientServer> server;
        // config specified server url
        QString url;
        QTime started;
        int failcount = 0;
        // pending settings to be submitted
        QJsonValue settings;
        // use of workspace folders allowed
        bool useWorkspace = false;
    };

    struct DocumentInfo {
        std::shared_ptr<LSPClientServer> server;
        // merged server config as obtain from various sources
        QJsonObject config;
        KTextEditor::Document *doc;
        QUrl url;
        qint64 version;
        bool open : 1;
        bool modified : 1;
        // used for incremental update (if non-empty)
        QList<LSPTextDocumentContentChangeEvent> changes;
    };

    LSPClientPlugin *m_plugin;
    QPointer<QObject> m_projectPlugin;
    // merged default and user config
    QJsonObject m_serverConfig;
    // root -> (mode -> server)
    QMap<QUrl, QMap<QString, ServerInfo>> m_servers;
    QHash<KTextEditor::Document *, DocumentInfo> m_docs;
    bool m_incrementalSync = false;
    LSPClientCapabilities m_clientCapabilities;

    // highlightingModeRegex => language id
    std::vector<std::pair<QRegularExpression, QString>> m_highlightingModeRegexToLanguageId;
    // cache of highlighting mode => language id, to avoid massive regex matching
    QHash<QString, QString> m_highlightingModeToLanguageIdCache;
    // whether to pass the language id (key) to server when opening document
    // most either do not care about the id, or can find out themselves
    // (and might get confused if we pass a not so accurate one)
    QHash<QString, bool> m_documentLanguageId;
    typedef QList<std::shared_ptr<LSPClientServer>> ServerList;

    // Servers which were not found to be installed. We use this
    // variable to avoid warning more than once
    QSet<QString> m_failedToFindServers;

public:
    LSPClientServerManagerImpl(LSPClientPlugin *plugin)
        : m_plugin(plugin)
    {
        connect(plugin, &LSPClientPlugin::update, this, &self_type::updateServerConfig);
        QTimer::singleShot(100, this, &self_type::updateServerConfig);

        // stay tuned on project situation
        auto app = KTextEditor::Editor::instance()->application();
        auto h = [this](const QString &name, KTextEditor::Plugin *plugin) {
            if (name == PROJECT_PLUGIN) {
                m_projectPlugin = plugin;
                monitorProjects(plugin);
            }
        };
        connect(app, &KTextEditor::Application::pluginCreated, this, h);
        auto projectPlugin = app->plugin(PROJECT_PLUGIN);
        m_projectPlugin = projectPlugin;
        monitorProjects(projectPlugin);
    }

    ~LSPClientServerManagerImpl() override
    {
        // stop everything as we go down
        // several stages;
        // stage 1; request shutdown of all servers (in parallel)
        // (give that some time)
        // stage 2; send TERM
        // stage 3; send KILL

        // stage 1

        /* some msleep are used below which is somewhat BAD as it blocks/hangs
         * the mainloop, however there is not much alternative:
         * + running an inner mainloop leads to event processing,
         *   which could trigger an unexpected sequence of 'events'
         *   such as (re)loading plugin that is currently still unloading
         *   (consider scenario of fast-clicking enable/disable of LSP plugin)
         * + could reduce or forego the sleep, but that increases chances
         *   on an unclean shutdown of LSP server, which may or may not
         *   be able to handle that properly (so let's try and be a polite
         *   client and try to avoid that to some degree)
         * So we are left with a minor sleep compromise ...
         */

        int count = 0;
        for (const auto &el : qAsConst(m_servers)) {
            for (const auto &si : el) {
                auto &s = si.server;
                if (!s) {
                    continue;
                }
                disconnect(s.get(), nullptr, this, nullptr);
                if (s->state() != LSPClientServer::State::None) {
                    ++count;
                    s->stop(-1, -1);
                }
            }
        }
        if (count) {
            QThread::msleep(500);
        } else {
            return;
        }

        // stage 2 and 3
        count = 0;
        for (count = 0; count < 2; ++count) {
            bool wait = false;
            for (const auto &el : qAsConst(m_servers)) {
                for (const auto &si : el) {
                    auto &s = si.server;
                    if (!s) {
                        continue;
                    }
                    wait = true;
                    s->stop(count == 0 ? 1 : -1, count == 0 ? -1 : 1);
                }
            }
            if (wait && count == 0) {
                QThread::msleep(100);
            }
        }
    }

    // map (highlight)mode to lsp languageId
    QString _languageId(const QString &mode)
    {
        // query cache first
        const auto cacheIt = m_highlightingModeToLanguageIdCache.find(mode);
        if (cacheIt != m_highlightingModeToLanguageIdCache.end()) {
            return cacheIt.value();
        }

        // match via regexes + cache result
        for (const auto &it : m_highlightingModeRegexToLanguageId) {
            if (it.first.match(mode).hasMatch()) {
                m_highlightingModeToLanguageIdCache[mode] = it.second;
                return it.second;
            }
        }

        // else: we have no matching server!
        m_highlightingModeToLanguageIdCache[mode] = QString();
        return QString();
    }

    QString languageId(KTextEditor::Document *doc)
    {
        if (!doc) {
            return {};
        }

        // prefer the mode over the highlighting to allow to
        // use known a highlighting with existing LSP
        // for a mode for a new language and own LSP
        // see bug 474887
        if (const auto langId = _languageId(doc->mode()); !langId.isEmpty()) {
            return langId;
        }

        return _languageId(doc->highlightingMode());
    }

    QObject *projectPluginView(KTextEditor::MainWindow *mainWindow)
    {
        return mainWindow->pluginView(PROJECT_PLUGIN);
    }

    QString documentLanguageId(KTextEditor::Document *doc)
    {
        auto langId = languageId(doc);
        const auto it = m_documentLanguageId.find(langId);
        // FIXME ?? perhaps use default false
        // most servers can find out much better on their own
        // (though it would actually have to be confirmed as such)
        bool useId = true;
        if (it != m_documentLanguageId.end()) {
            useId = it.value();
        }

        return useId ? langId : QString();
    }

    void setIncrementalSync(bool inc) override
    {
        m_incrementalSync = inc;
    }

    LSPClientCapabilities &clientCapabilities() override
    {
        return m_clientCapabilities;
    }

    std::shared_ptr<LSPClientServer> findServer(KTextEditor::View *view, bool updatedoc = true) override
    {
        if (!view) {
            return nullptr;
        }

        auto document = view->document();
        if (!document || document->url().isEmpty()) {
            return nullptr;
        }

        auto it = m_docs.find(document);
        auto server = it != m_docs.end() ? it->server : nullptr;
        if (!server) {
            QJsonObject serverConfig;
            if ((server = _findServer(view, document, serverConfig))) {
                trackDocument(document, server, serverConfig);
            }
        }

        if (server && updatedoc) {
            update(server.get(), false);
        }
        return server;
    }

    virtual QJsonValue findServerConfig(KTextEditor::Document *document) override
    {
        // check if document has been seen/processed by now
        auto it = m_docs.find(document);
        auto config = it != m_docs.end() ? QJsonValue(it->config) : QJsonValue::Null;
        return config;
    }

    // restart a specific server or all servers if server == nullptr
    void restart(LSPClientServer *server) override
    {
        ServerList servers;
        // find entry for server(s) and move out
        for (auto &m : m_servers) {
            for (auto it = m.begin(); it != m.end();) {
                if (!server || it->server.get() == server) {
                    servers.push_back(it->server);
                    it = m.erase(it);
                } else {
                    ++it;
                }
            }
        }
        restart(servers, server == nullptr);
    }

    qint64 revision(KTextEditor::Document *doc) override
    {
        auto it = m_docs.find(doc);
        return it != m_docs.end() ? it->version : -1;
    }

    LSPClientRevisionSnapshot *snapshot(LSPClientServer *server) override
    {
        auto result = new LSPClientRevisionSnapshotImpl;
        for (auto it = m_docs.begin(); it != m_docs.end(); ++it) {
            if (it->server.get() == server) {
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
        // inform interested view(er) which will decide how/where to show
        Q_EMIT m_plugin->showMessage(level, msg);
    }

    // caller ensures that servers are no longer present in m_servers
    void restart(const ServerList &servers, bool reload = false)
    {
        // close docs
        for (const auto &server : servers) {
            if (!server) {
                continue;
            }
            // controlling server here, so disable usual state tracking response
            disconnect(server.get(), nullptr, this, nullptr);
            for (auto it = m_docs.begin(); it != m_docs.end();) {
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
        auto stopservers = [servers](int t, int k) {
            for (const auto &server : servers) {
                if (server) {
                    server->stop(t, k);
                }
            }
        };

        // trigger server shutdown now
        stopservers(-1, -1);

        // initiate delayed stages (TERM and KILL)
        // async, so give a bit more time
        QTimer::singleShot(2 * TIMEOUT_SHUTDOWN, this, [stopservers]() {
            stopservers(1, -1);
        });
        QTimer::singleShot(4 * TIMEOUT_SHUTDOWN, this, [stopservers]() {
            stopservers(-1, 1);
        });

        // as for the start part
        // trigger interested parties, which will again request a server as needed
        // let's delay this; less chance for server instances to trip over each other
        QTimer::singleShot(6 * TIMEOUT_SHUTDOWN, this, [this, reload]() {
            // this may be a good time to refresh server config
            if (reload) {
                // will also trigger as mentioned above
                updateServerConfig();
            } else {
                Q_EMIT serverChanged();
            }
        });
    }

    void onStateChanged(LSPClientServer *server)
    {
        if (server->state() == LSPClientServer::State::Running) {
            // send settings if pending
            ServerInfo *info = nullptr;
            for (auto &m : m_servers) {
                for (auto &si : m) {
                    if (si.server.get() == server) {
                        info = &si;
                        break;
                    }
                }
            }
            if (info && !info->settings.isUndefined()) {
                server->didChangeConfiguration(info->settings);
            }
            // provide initial workspace folder situation
            // this is done here because the folder notification pre-dates
            // the workspaceFolders property in 'initialize'
            // there is also no way to know whether the server supports that
            // and in fact some servers do "support workspace folders" (e.g. notification)
            // but do not know about the 'initialize' property
            // so, in summary, the notification is used here rather than the property
            const auto &caps = server->capabilities();
            if (caps.workspaceFolders.changeNotifications && info && info->useWorkspace) {
                if (auto folders = currentWorkspaceFolders(); !folders.isEmpty()) {
                    server->didChangeWorkspaceFolders(folders, {});
                }
            }
            // clear for normal operation
            Q_EMIT serverChanged();
        } else if (server->state() == LSPClientServer::State::None) {
            // went down
            // find server info to see how bad this is
            // if this is an occasional termination/crash ... ok then
            // if this happens quickly (bad/missing server, wrong cmdline/config), then no restart
            std::shared_ptr<LSPClientServer> sserver;
            QString url;
            bool retry = true;
            for (auto &m : m_servers) {
                for (auto &si : m) {
                    if (si.server.get() == server) {
                        url = si.url;
                        if (si.started.secsTo(QTime::currentTime()) < 60) {
                            ++si.failcount;
                        }
                        // clear the entry, which will be re-filled if needed
                        // otherwise, leave it in place as a dead mark not to re-create one in _findServer
                        if (si.failcount < 2) {
                            std::swap(sserver, si.server);
                        } else {
                            sserver = si.server;
                            retry = false;
                        }
                    }
                }
            }
            auto action = retry ? i18n("Restarting") : i18n("NOT Restarting");
            showMessage(i18n("Server terminated unexpectedly ... %1 [%2] [homepage: %3] ", action, server->cmdline().join(QLatin1Char(' ')), url),
                        KTextEditor::Message::Warning);
            if (sserver) {
                // sserver might still be in m_servers
                // but since it died already bringing it down will have no (ill) effect
                restart({sserver});
            }
        }
    }

    std::shared_ptr<LSPClientServer> _findServer(KTextEditor::View *view, KTextEditor::Document *document, QJsonObject &mergedConfig)
    {
        // compute the LSP standardized language id, none found => no change
        auto langId = languageId(document);
        if (langId.isEmpty()) {
            return nullptr;
        }

        // get project plugin infos if available
        const auto projectBase = Utils::projectBaseDirForDocument(document);
        const auto projectMap = Utils::projectMapForDocument(document);

        // merge with project specific
        auto projectConfig = QJsonDocument::fromVariant(projectMap).object().value(QStringLiteral("lspclient")).toObject();
        auto serverConfig = json::merge(m_serverConfig, projectConfig);

        // locate server config
        QJsonValue config;
        QSet<QString> used;
        // reduce langId
        auto realLangId = langId;
        while (true) {
            qCInfo(LSPCLIENT) << "language id " << langId;
            used << langId;
            config = serverConfig.value(QStringLiteral("servers")).toObject().value(langId);
            if (config.isObject()) {
                const auto &base = config.toObject().value(QStringLiteral("use")).toString();
                // basic cycle detection
                if (!base.isEmpty() && !used.contains(base)) {
                    langId = base;
                    continue;
                }
            }
            break;
        }

        if (!config.isObject()) {
            return nullptr;
        }

        // merge global settings
        serverConfig = json::merge(serverConfig.value(QStringLiteral("global")).toObject(), config.toObject());

        // used for variable substitution in the sequl
        // NOTE that also covers a form of environment substitution using %{ENV:XYZ}
        auto editor = KTextEditor::Editor::instance();

        std::optional<QString> rootpath;
        const auto rootv = serverConfig.value(QStringLiteral("root"));
        if (rootv.isString()) {
            auto sroot = rootv.toString();
            sroot = editor->expandText(sroot, view);
            if (QDir::isAbsolutePath(sroot)) {
                rootpath = sroot;
            } else if (!projectBase.isEmpty()) {
                rootpath = QDir(projectBase).absoluteFilePath(sroot);
            } else if (sroot.isEmpty()) {
                // empty root; so we are convinced the server can handle null rootUri
                rootpath = QString();
            } else if (const auto url = document->url(); url.isValid() && url.isLocalFile()) {
                // likewise, but use safer traditional approach and specify rootUri
                rootpath = QDir(QFileInfo(url.toLocalFile()).absolutePath()).absoluteFilePath(sroot);
            }
        }

        /**
         * no explicit set root dir? search for a matching root based on some name filters
         * this is required for some LSP servers like rls that don't handle that on their own like
         * clangd does
         */
        if (!rootpath) {
            const auto fileNamesForDetection = indicationDataToStringList(serverConfig.value(QStringLiteral("rootIndicationFileNames")));
            const auto filePatternsForDetection = indicationDataToStringList(serverConfig.value(QStringLiteral("rootIndicationFilePatterns")));
            const auto root = findRootForDocument(document, fileNamesForDetection, filePatternsForDetection);
            if (!root.isEmpty()) {
                rootpath = root;
            }
        }

        // just in case ... ensure normalized result
        if (rootpath && !rootpath->isEmpty()) {
            auto cpath = QFileInfo(*rootpath).canonicalFilePath();
            if (!cpath.isEmpty()) {
                rootpath = cpath;
            }
        }

        // is it actually safe/reasonable to use workspaces?
        // in practice, (at this time) servers do do not quite consider or support all that
        // so in that regard workspace folders represents a bit of "spec endulgance"
        // (along with quite some other aspects for that matter)
        //
        // if a server was/is able to handle a "generic root",
        //   let's assume it is safe to consider workspace folders if it explicitly claims such support
        // if, however, an explicit root was/is necessary,
        //   let's assume not safe
        // in either case, let configuration explicitly specify this
        bool useWorkspace = serverConfig.value(QStringLiteral("useWorkspace")).toBool(!rootpath ? true : false);

        // last fallback: home directory
        if (!rootpath) {
            rootpath = QDir::homePath();
        }

        auto root = rootpath && !rootpath->isEmpty() ? QUrl::fromLocalFile(*rootpath) : QUrl();
        auto &serverinfo = m_servers[root][langId];
        auto &server = serverinfo.server;

        // maybe there is a server with other root that is workspace capable
        if (!server && useWorkspace) {
            for (const auto &l : qAsConst(m_servers)) {
                // for (auto it = l.begin(); it != l.end(); ++it) {
                auto it = l.find(langId);
                if (it != l.end()) {
                    if (auto oserver = it->server) {
                        const auto &caps = oserver->capabilities();
                        if (caps.workspaceFolders.supported && caps.workspaceFolders.changeNotifications && it->useWorkspace) {
                            // so this server can handle workspace folders and should know about project root
                            server = oserver;
                            break;
                        }
                    }
                }
            }
        }

        QStringList cmdline;
        if (!server) {
            // need to find command line for server
            // choose debug command line for debug mode, fallback to command
            auto vcmdline = serverConfig.value(m_plugin->m_debugMode ? QStringLiteral("commandDebug") : QStringLiteral("command"));
            if (vcmdline.isUndefined()) {
                vcmdline = serverConfig.value(QStringLiteral("command"));
            }

            auto scmdline = vcmdline.toString();
            if (!scmdline.isEmpty()) {
                cmdline = scmdline.split(QLatin1Char(' '));
            } else {
                const auto cmdOpts = vcmdline.toArray();
                for (const auto &c : cmdOpts) {
                    cmdline.push_back(c.toString());
                }
            }

            // some more expansion and substitution
            // unlikely to be used here, but anyway
            for (auto &e : cmdline) {
                e = editor->expandText(e, view);
            }
        }

        if (cmdline.length() > 0) {
            // always update some info
            // (even if eventually no server found/started)
            serverinfo.settings = serverConfig.value(QStringLiteral("settings"));
            serverinfo.started = QTime::currentTime();
            serverinfo.url = serverConfig.value(QStringLiteral("url")).toString();
            // leave failcount as-is
            serverinfo.useWorkspace = useWorkspace;

            // ensure we always only take the server executable from the PATH or user defined paths
            // QProcess will take the executable even just from current working directory without this => BAD
            auto cmd = safeExecutableName(cmdline[0]);

            // optionally search in supplied path(s)
            const auto vpath = serverConfig.value(QStringLiteral("path")).toArray();
            if (cmd.isEmpty() && !vpath.isEmpty()) {
                // collect and expand in case home dir or other (environment) variable reference is used
                QStringList path;
                for (const auto &e : vpath) {
                    auto p = e.toString();
                    p = editor->expandText(p, view);
                    path.push_back(p);
                }
                cmd = safeExecutableName(cmdline[0], path);
            }

            // we can only start the stuff if we did find the binary in the paths
            if (!cmd.isEmpty()) {
                // use full path to avoid security issues
                cmdline[0] = cmd;
            } else {
                if (!m_failedToFindServers.contains(cmdline[0])) {
                    m_failedToFindServers.insert(cmdline[0]);
                    // we didn't find the server binary at all!
                    QString message = i18n("Failed to find server binary: %1", cmdline[0]);
                    const auto url = serverConfig.value(QStringLiteral("url")).toString();
                    if (!url.isEmpty()) {
                        message += QStringLiteral("\n") + i18n("Please check your PATH for the binary");
                        message += QStringLiteral("\n") + i18n("See also %1 for installation or details", url);
                    }
                    showMessage(message, KTextEditor::Message::Warning);
                }
                // clear to cut branch below
                cmdline.clear();
            }
        }

        // check if allowed to start, function will query user if needed and emit messages
        if (cmdline.length() > 0 && !m_plugin->isCommandLineAllowed(cmdline)) {
            cmdline.clear();
        }

        // made it here with a command line; spin up server
        if (cmdline.length() > 0) {
            // an empty list is always passed here (or null)
            // the initial list is provided/updated using notification after start
            // since that is what a server is more aware of
            // and should support if it declares workspace folder capable
            // (as opposed to the new initialization property)
            LSPClientServer::FoldersType folders;
            if (useWorkspace) {
                folders = QList<LSPWorkspaceFolder>();
            }
            // spin up using currently configured client capabilities
            auto &caps = m_clientCapabilities;
            // extract some more additional config
            auto completionOverride = parseTriggerOverride(serverConfig.value(QStringLiteral("completionTriggerCharacters")));
            auto signatureOverride = parseTriggerOverride(serverConfig.value(QStringLiteral("signatureTriggerCharacters")));
            // request server and setup
            server.reset(new LSPClientServer(cmdline,
                                             root,
                                             realLangId,
                                             serverConfig.value(QStringLiteral("initializationOptions")),
                                             {folders, caps, completionOverride, signatureOverride}));
            connect(server.get(), &LSPClientServer::stateChanged, this, &self_type::onStateChanged, Qt::UniqueConnection);
            if (!server->start(m_plugin->m_debugMode)) {
                QString message = i18n("Failed to start server: %1", cmdline.join(QLatin1Char(' ')));
                const auto url = serverConfig.value(QStringLiteral("url")).toString();
                if (!url.isEmpty()) {
                    message += QStringLiteral("\n") + i18n("Please check your PATH for the binary");
                    message += QStringLiteral("\n") + i18n("See also %1 for installation or details", url);
                }
                showMessage(message, KTextEditor::Message::Warning);
            } else {
                showMessage(i18n("Started server %2: %1", cmdline.join(QLatin1Char(' ')), serverDescription(server.get())), KTextEditor::Message::Positive);
                using namespace std::placeholders;
                server->connect(server.get(), &LSPClientServer::logMessage, this, std::bind(&self_type::onMessage, this, true, _1));
                server->connect(server.get(), &LSPClientServer::showMessage, this, std::bind(&self_type::onMessage, this, false, _1));
                server->connect(server.get(), &LSPClientServer::workDoneProgress, this, &self_type::onWorkDoneProgress);
                server->connect(server.get(), &LSPClientServer::workspaceFolders, this, &self_type::onWorkspaceFolders, Qt::UniqueConnection);
                server->connect(server.get(), &LSPClientServer::showMessageRequest, this, &self_type::showMessageRequest);
            }
        }
        // set out param value
        mergedConfig = serverConfig;
        return (server && server->state() == LSPClientServer::State::Running) ? server : nullptr;
    }

    void updateServerConfig()
    {
        // default configuration, compiled into plugin resource, reading can't fail
        QFile defaultConfigFile(QStringLiteral(":/lspclient/settings.json"));
        defaultConfigFile.open(QIODevice::ReadOnly);
        Q_ASSERT(defaultConfigFile.isOpen());
        m_serverConfig = QJsonDocument::fromJson(defaultConfigFile.readAll()).object();

        // consider specified configuration if existing
        const auto configPath = m_plugin->configPath().toLocalFile();
        if (!configPath.isEmpty() && QFile::exists(configPath)) {
            QFile f(configPath);
            if (f.open(QIODevice::ReadOnly)) {
                const auto data = f.readAll();
                if (!data.isEmpty()) {
                    QJsonParseError error{};
                    auto json = QJsonDocument::fromJson(data, &error);
                    if (error.error == QJsonParseError::NoError) {
                        if (json.isObject()) {
                            m_serverConfig = json::merge(m_serverConfig, json.object());
                        } else {
                            showMessage(i18n("Failed to parse server configuration '%1': no JSON object", configPath), KTextEditor::Message::Error);
                        }
                    } else {
                        showMessage(i18n("Failed to parse server configuration '%1': %2", configPath, error.errorString()), KTextEditor::Message::Error);
                    }
                }
            } else {
                showMessage(i18n("Failed to read server configuration: %1", configPath), KTextEditor::Message::Error);
            }
        }

        // build regex of highlightingMode => language id
        m_highlightingModeRegexToLanguageId.clear();
        m_highlightingModeToLanguageIdCache.clear();
        const auto servers = m_serverConfig.value(QLatin1String("servers")).toObject();
        for (auto it = servers.begin(); it != servers.end(); ++it) {
            // get highlighting mode regex for this server, if not set, fallback to just the name
            const auto &server = it.value().toObject();
            QString highlightingModeRegex = server.value(QLatin1String("highlightingModeRegex")).toString();
            if (highlightingModeRegex.isEmpty()) {
                highlightingModeRegex = it.key();
            }
            m_highlightingModeRegexToLanguageId.emplace_back(QRegularExpression(highlightingModeRegex, QRegularExpression::CaseInsensitiveOption), it.key());
            // should we use the languageId in didOpen
            auto docLanguageId = server.value(QLatin1String("documentLanguageId"));
            if (docLanguageId.isBool()) {
                m_documentLanguageId[it.key()] = docLanguageId.toBool();
            }
        }
        m_failedToFindServers.clear();

        // we could (but do not) perform restartAll here;
        // for now let's leave that up to user
        // but maybe we do have a server now where not before, so let's signal
        Q_EMIT serverChanged();
    }

    void trackDocument(KTextEditor::Document *doc, const std::shared_ptr<LSPClientServer> &server, QJsonObject serverConfig)
    {
        auto it = m_docs.find(doc);
        if (it == m_docs.end()) {
            // TODO: Further simplify once we are Qt6
            // track document
            connect(doc, &KTextEditor::Document::documentUrlChanged, this, &self_type::untrack, Qt::UniqueConnection);
            it = m_docs.insert(doc, {server, serverConfig, doc, doc->url(), 0, false, false, {}});
            connect(doc, &KTextEditor::Document::highlightingModeChanged, this, &self_type::untrack, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::aboutToClose, this, &self_type::untrack, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::destroyed, this, &self_type::untrack, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::textChanged, this, &self_type::onTextChanged, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::documentSavedOrUploaded, this, &self_type::onDocumentSaved, Qt::UniqueConnection);
            // in case of incremental change
            connect(doc, &KTextEditor::Document::textInserted, this, &self_type::onTextInserted, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::textRemoved, this, &self_type::onTextRemoved, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::lineWrapped, this, &self_type::onLineWrapped, Qt::UniqueConnection);
            connect(doc, &KTextEditor::Document::lineUnwrapped, this, &self_type::onLineUnwrapped, Qt::UniqueConnection);
        } else {
            it->server = server;
        }
    }

    decltype(m_docs)::iterator _close(decltype(m_docs)::iterator it, bool remove)
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
        _close(qobject_cast<KTextEditor::Document *>(doc), true);
        Q_EMIT serverChanged();
    }

    void close(KTextEditor::Document *doc)
    {
        _close(doc, false);
    }

    void update(const decltype(m_docs)::iterator &it, bool force)
    {
        if (it != m_docs.end() && it->server) {
            auto doc = it.key();
            it->version = it->doc->revision();

            if (!m_incrementalSync) {
                it->changes.clear();
            }
            if (it->open) {
                if (it->modified || force) {
                    (it->server)->didChange(it->url, it->version, (it->changes.empty()) ? doc->text() : QString(), it->changes);
                }
            } else {
                (it->server)->didOpen(it->url, it->version, documentLanguageId(doc), doc->text());
                it->open = true;
            }
            it->modified = false;
            it->changes.clear();
        }
    }

    void update(KTextEditor::Document *doc, bool force) override
    {
        update(m_docs.find(doc), force);
    }

    void update(LSPClientServer *server, bool force)
    {
        for (auto it = m_docs.begin(); it != m_docs.end(); ++it) {
            if (it->server.get() == server) {
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

    DocumentInfo *getDocumentInfo(KTextEditor::Document *doc)
    {
        if (!m_incrementalSync) {
            return nullptr;
        }

        auto it = m_docs.find(doc);
        if (it != m_docs.end() && it->server) {
            const auto &caps = it->server->capabilities();
            if (caps.textDocumentSync.change == LSPDocumentSyncKind::Incremental) {
                return &(*it);
            }
        }
        return nullptr;
    }

    void onTextInserted(KTextEditor::Document *doc, const KTextEditor::Cursor &position, const QString &text)
    {
        auto info = getDocumentInfo(doc);
        if (info) {
            info->changes.push_back({LSPRange{position, position}, text});
        }
    }

    void onTextRemoved(KTextEditor::Document *doc, const KTextEditor::Range &range, const QString &text)
    {
        (void)text;
        auto info = getDocumentInfo(doc);
        if (info) {
            info->changes.push_back({range, QString()});
        }
    }

    void onLineWrapped(KTextEditor::Document *doc, const KTextEditor::Cursor &position)
    {
        // so a 'newline' has been inserted at position
        // could have been UNIX style or other kind, let's ask the document
        auto text = doc->text({position, {position.line() + 1, 0}});
        onTextInserted(doc, position, text);
    }

    void onLineUnwrapped(KTextEditor::Document *doc, int line)
    {
        // lines line-1 and line got replaced by current content of line-1
        Q_ASSERT(line > 0);
        auto info = getDocumentInfo(doc);
        if (info) {
            LSPRange oldrange{{line - 1, 0}, {line + 1, 0}};
            LSPRange newrange{{line - 1, 0}, {line, 0}};
            auto text = doc->text(newrange);
            info->changes.push_back({oldrange, text});
        }
    }

    void onDocumentSaved(KTextEditor::Document *doc, bool saveAs)
    {
        if (!saveAs) {
            auto it = m_docs.find(doc);
            if (it != m_docs.end() && it->server) {
                auto server = it->server;
                const auto &saveOptions = server->capabilities().textDocumentSync.save;
                if (saveOptions) {
                    server->didSave(doc->url(), saveOptions->includeText ? doc->text() : QString());
                }
            }
        }
    }

    void onMessage(bool isLog, const LSPLogMessageParams &params)
    {
        // determine server description
        auto server = qobject_cast<LSPClientServer *>(sender());
        if (isLog) {
            Q_EMIT serverLogMessage(server, params);
        } else {
            Q_EMIT serverShowMessage(server, params);
        }
    }

    void onWorkDoneProgress(const LSPWorkDoneProgressParams &params)
    {
        // determine server description
        auto server = qobject_cast<LSPClientServer *>(sender());
        Q_EMIT serverWorkDoneProgress(server, params);
    }

    static std::pair<QString, QString> getProjectNameDir(const QObject *kateProject)
    {
        return {kateProject->property("name").toString(), kateProject->property("baseDir").toString()};
    }

    QList<LSPWorkspaceFolder> currentWorkspaceFolders()
    {
        QList<LSPWorkspaceFolder> folders;
        if (m_projectPlugin) {
            auto projects = m_projectPlugin->property("projects").value<QObjectList>();
            for (auto proj : projects) {
                auto props = getProjectNameDir(proj);
                folders.push_back(workspaceFolder(props.second, props.first));
            }
        }
        return folders;
    }

    static LSPWorkspaceFolder workspaceFolder(const QString &baseDir, const QString &name)
    {
        return {QUrl::fromLocalFile(baseDir), name};
    }

    void updateWorkspace(bool added, const QObject *project)
    {
        auto props = getProjectNameDir(project);
        auto &name = props.first;
        auto &baseDir = props.second;
        qCInfo(LSPCLIENT) << "update workspace" << added << baseDir << name;
        for (const auto &u : qAsConst(m_servers)) {
            for (const auto &si : u) {
                if (auto server = si.server) {
                    const auto &caps = server->capabilities();
                    if (caps.workspaceFolders.changeNotifications && si.useWorkspace) {
                        auto wsfolder = workspaceFolder(baseDir, name);
                        QList<LSPWorkspaceFolder> l{wsfolder}, empty;
                        server->didChangeWorkspaceFolders(added ? l : empty, added ? empty : l);
                    }
                }
            }
        }
    }

    Q_SLOT void onProjectAdded(QObject *project)
    {
        updateWorkspace(true, project);
    }

    Q_SLOT void onProjectRemoved(QObject *project)
    {
        updateWorkspace(false, project);
    }

    void monitorProjects(KTextEditor::Plugin *projectPlugin)
    {
        if (projectPlugin) {
            // clang-format off
            auto c = connect(projectPlugin,
                        SIGNAL(projectAdded(QObject*)),
                        this,
                        SLOT(onProjectAdded(QObject*)),
                        Qt::UniqueConnection);
            c = connect(projectPlugin,
                        SIGNAL(projectRemoved(QObject*)),
                        this,
                        SLOT(onProjectRemoved(QObject*)),
                        Qt::UniqueConnection);
            // clang-format on
        }
    }

    void onWorkspaceFolders(const WorkspaceFoldersReplyHandler &h, bool &handled)
    {
        if (handled) {
            return;
        }

        auto folders = currentWorkspaceFolders();
        h(folders);

        handled = true;
    }
};

std::shared_ptr<LSPClientServerManager> LSPClientServerManager::new_(LSPClientPlugin *plugin)
{
    return std::shared_ptr<LSPClientServerManager>(new LSPClientServerManagerImpl(plugin));
}

#include "lspclientservermanager.moc"
#include "moc_lspclientservermanager.cpp"
