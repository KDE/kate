/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpclientservermanager.h"
#include "acpclient_debug.h"
#include "acpclientplugin.h"
#include "acpclientprotocol.h"

#include <json_utils.h>

#include <KLocalizedString>

#include <QDir>
#include <QJsonArray>
#include <QJsonObject>

ACPClientServerManager::ACPClientServerManager(ACPClientPlugin *plugin, QObject *parent)
    : QObject(parent)
    , m_plugin(plugin)
{
    // we load the server config on change, plugin will emit that once on startup, too
    connect(m_plugin, &ACPClientPlugin::update, this, &ACPClientServerManager::loadDefaultServers);
}

void ACPClientServerManager::loadDefaultServers()
{
    // default configuration, compiled into plugin resource, reading can't fail
    QFile defaultConfigFile(QStringLiteral(":/kateacpclient/settings.json"));
    if (!defaultConfigFile.open(QIODevice::ReadOnly)) {
        Q_UNREACHABLE();
    }
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

    // create the servers for the config
    // FIXME: what to do if we have some that are already running on reload?

    if (m_serverConfig.contains(u"servers") && m_serverConfig[u"servers"].isArray()) {
        QJsonArray servers = m_serverConfig[u"servers"].toArray();
        for (const QJsonValue &v : servers) {
            if (v.isObject()) {
                QJsonObject serverObj = v.toObject();
                ACPClientServer::ServerInfo info;

                info.name = serverObj[u"name"].toString();
                info.version = serverObj[u"version"].toString();
                info.command = serverObj[u"command"].toString();

                if (serverObj.contains(u"arguments") && serverObj[u"arguments"].isArray()) {
                    QJsonArray args = serverObj[u"arguments"].toArray();
                    for (const QJsonValue &arg : args) {
                        info.arguments.append(arg.toString());
                    }
                }

                // Create the server but don't auto-start here (handled by createServer)
                createServer(info);
            }
        }
    }
}

ACPClientServerManager::~ACPClientServerManager()
{
    // Clean up all servers
    for (auto &server : m_servers) {
        server->stop();
    }
    m_servers.clear();
    qCDebug(ACPCLIENT) << "ACPClientServerManager destroyed";
}

ACPClientServer *ACPClientServerManager::createServer(const ACPClientServer::ServerInfo &info)
{
    qCDebug(ACPCLIENT) << "Creating server:" << info.name;

    auto server = std::make_unique<ACPClientServer>(info, this);
    ACPClientServer *serverPtr = server.get();

    connect(serverPtr, &ACPClientServer::initialized, this, &ACPClientServerManager::onServerInitialized);
    connect(serverPtr, &ACPClientServer::disconnected, this, &ACPClientServerManager::onServerDisconnected);
    connect(serverPtr, &ACPClientServer::errorOccurred, this, &ACPClientServerManager::onServerError);
    connect(serverPtr, &ACPClientServer::messageReceived, this, &ACPClientServerManager::onServerMessageReceived);

    m_servers.push_back(std::move(server));

    Q_EMIT serverAdded(serverPtr);

    return serverPtr;
}

QList<ACPClientServer *> ACPClientServerManager::servers() const
{
    QList<ACPClientServer *> result;
    for (const auto &server : m_servers) {
        result.append(server.get());
    }
    return result;
}

ACPClientServer *ACPClientServerManager::server(const QString &name) const
{
    for (const auto &server : m_servers) {
        if (server->info().name == name) {
            return server.get();
        }
    }
    return nullptr;
}

void ACPClientServerManager::removeServer(ACPClientServer *server)
{
    for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
        if (it->get() == server) {
            server->stop();

            if (m_activeServer == server) {
                m_activeServer = nullptr;
                m_activeServerName.clear();
                Q_EMIT activeServerChanged(nullptr);
            }

            Q_EMIT serverRemoved(server);
            m_servers.erase(it);
            break;
        }
    }
}

ACPClientServer *ACPClientServerManager::activeServer() const
{
    if (m_activeServer && m_activeServer->state() == ACPClientServer::ServerState::Initialized) {
        return m_activeServer;
    }

    // Try to find the first initialized server
    for (const auto &server : m_servers) {
        if (server->state() == ACPClientServer::ServerState::Initialized) {
            return server.get();
        }
    }

    return nullptr;
}

void ACPClientServerManager::setActiveServer(const QString &name)
{
    ACPClientServer *newServer = server(name);
    if (newServer && newServer != m_activeServer) {
        m_activeServer = newServer;
        m_activeServerName = name;
        Q_EMIT activeServerChanged(m_activeServer);
    }
}

bool ACPClientServerManager::supportsLoadSession() const
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        return false;
    }
    return server->capabilities().loadSession;
}

bool ACPClientServerManager::supportsResumeSession() const
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        return false;
    }
    return server->capabilities().sessionCapabilities.resume;
}

QString ACPClientServerManager::createSession()
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return QString();
    }

    ACP::SessionNewParams params;
    // Set cwd to current working directory
    params.cwd = QDir::currentPath();
    // Set mcpServers to empty array (vibe-acp expects a list)
    params.mcpServers = QJsonArray();
    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionNewRequest(params, requestId);

    // Track the request for response - store the requestId and emit signal when response arrives
    // We use a map to track pending session creations
    m_pendingSessionRequests[requestId] = requestId;

    server->sendMessage(request);

    // Return empty for now - the actual session ID will come via signal
    return QString();
}

QString ACPClientServerManager::loadSession(const QString &sessionId)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return QString();
    }

    ACP::SessionLoadParams params;
    params.sessionId = sessionId;
    params.cwd = QDir::currentPath();
    params.mcpServers = QJsonArray();

    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionLoadRequest(params, requestId);

    // Track the request
    m_pendingSessionRequests[requestId] = requestId;

    server->sendMessage(request);

    return QString();
}

QString ACPClientServerManager::resumeSession(const QString &sessionId)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return QString();
    }

    ACP::SessionResumeParams params;
    params.sessionId = sessionId;
    params.cwd = QDir::currentPath();
    params.mcpServers = QJsonArray();

    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionResumeRequest(params, requestId);

    // Track the request
    m_pendingSessionRequests[requestId] = requestId;

    server->sendMessage(request);

    return QString();
}

void ACPClientServerManager::closeSession(const QString &sessionId)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    ACP::SessionCloseParams params;
    params.sessionId = sessionId;

    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionCloseRequest(params, requestId);

    server->sendMessage(request);
    Q_EMIT sessionClosed(sessionId);
}

void ACPClientServerManager::sendPrompt(const QString &sessionId, const QString &message)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    ACP::SessionPromptParams params;
    params.sessionId = sessionId;

    // Create a text content block
    ACP::ContentBlock textBlock;
    textBlock.type = ACP::contentTypeText();
    textBlock.text = message;
    params.prompt.append(textBlock);

    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionPromptRequest(params, requestId);

    server->sendMessage(request);
}

void ACPClientServerManager::listSessions()
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    ACP::ListSessionsRequest params;
    // Use default values: no limit, no offset
    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionListRequest(params, requestId);

    // Track the request type
    m_pendingRequests[requestId] = PendingRequestType::SessionList;

    server->sendMessage(request);
}

void ACPClientServerManager::deleteSession(const QString &sessionId)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    ACP::SessionDeleteParams params;
    params.sessionId = sessionId;
    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionDeleteRequest(params, requestId);

    server->sendMessage(request);
    Q_EMIT sessionDeleted(sessionId);
}

void ACPClientServerManager::listTools()
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createToolsListRequest(requestId);

    server->sendMessage(request);
}

void ACPClientServerManager::callTool(const QString &toolId, const QJsonObject &arguments)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createToolsCallRequest(toolId, arguments, requestId);

    server->sendMessage(request);
    Q_EMIT toolCallCompleted(requestId, QJsonObject());
}

void ACPClientServerManager::onServerInitialized(const ACP::InitializeResult &result)
{
    ACPClientServer *server = qobject_cast<ACPClientServer *>(sender());
    if (server) {
        qCDebug(ACPCLIENT) << "Server initialized:" << server->info().name << "Agent:" << result.agentInfo.name << "Version:" << result.agentInfo.version
                           << "Protocol:" << result.protocolVersion;

        // If no server is currently active, or if the active server is not initialized,
        // set this server as the active server
        if (!m_activeServer || m_activeServer->state() != ACPClientServer::ServerState::Initialized) {
            m_activeServer = server;
            m_activeServerName = server->info().name;
        }

        // Always emit activeServerChanged when a server initializes
        // This ensures the session list is queried after the server is ready
        Q_EMIT activeServerChanged(m_activeServer);
    }
}

void ACPClientServerManager::onServerDisconnected()
{
    ACPClientServer *server = qobject_cast<ACPClientServer *>(sender());
    if (server) {
        qCDebug(ACPCLIENT) << "Server disconnected:" << server->info().name;

        if (m_activeServer == server) {
            m_activeServer = nullptr;
            Q_EMIT activeServerChanged(nullptr);
        }
    }
}

void ACPClientServerManager::onServerError(const QString &error)
{
    ACPClientServer *server = qobject_cast<ACPClientServer *>(sender());
    if (server) {
        qCWarning(ACPCLIENT) << "Server error from" << server->info().name << ":" << error;
    }
    Q_EMIT errorOccurred(error);
}

void ACPClientServerManager::onServerMessageReceived(const QJsonDocument &message)
{
    QJsonObject obj = message.object();

    // Check for session/request_permission method
    if (obj.contains(u"method") && obj[u"method"].toString() == ACP::methodSessionRequestPermission()) {
        qCDebug(ACPCLIENT) << "Received permission request:" << message.toJson();
        handlePermissionRequest(message);
        return;
    }

    ACP::ACPMessage parsedMessage;
    if (ACP::ACPProtocol::parseMessage(message, parsedMessage)) {
        // Check for session responses by matching the request ID
        if (parsedMessage.isResponse && parsedMessage.id != 0) {
            // Check if this is a response to a session request
            if (m_pendingSessionRequests.find(parsedMessage.id) != m_pendingSessionRequests.end()) {
                QJsonObject result = parsedMessage.result;
                if (result.contains(u"sessionId")) {
                    QString sessionId = result[u"sessionId"].toString();
                    qCDebug(ACPCLIENT) << "Session response received for ID:" << sessionId;
                    m_pendingSessionRequests.erase(parsedMessage.id);
                    Q_EMIT sessionCreated(sessionId);
                    return;
                }
                // Remove from pending even if there was an error
                m_pendingSessionRequests.erase(parsedMessage.id);
            }
        }

        // Check for session/load response
        if (parsedMessage.method == ACP::methodSessionLoad() && parsedMessage.isResponse && parsedMessage.id != 0) {
            m_pendingSessionRequests.erase(parsedMessage.id);
            // session/load response has null result on success
            Q_EMIT sessionLoaded(parsedMessage.id != 0 ? QString() : QString()); // Session ID should be from the request
            return;
        }

        // Check for session/resume response
        if (parsedMessage.method == ACP::methodSessionResume() && parsedMessage.isResponse && parsedMessage.id != 0) {
            m_pendingSessionRequests.erase(parsedMessage.id);
            Q_EMIT sessionResumed(parsedMessage.id != 0 ? QString() : QString());
            return;
        }

        // Check for session/close response
        if (parsedMessage.method == ACP::methodSessionClose() && parsedMessage.isResponse) {
            Q_EMIT sessionClosed(parsedMessage.id != 0 ? QString() : QString());
            return;
        }

        // Check for session/list response
        if (parsedMessage.isResponse && parsedMessage.id != 0) {
            auto it = m_pendingRequests.find(parsedMessage.id);
            if (it != m_pendingRequests.end()) {
                if (it->second == PendingRequestType::SessionList) {
                    qCDebug(ACPCLIENT) << "Received session/list response:" << parsedMessage.result;

                    // Try to extract sessions from different possible locations
                    QJsonArray sessions;
                    if (parsedMessage.result.contains(u"sessions") && parsedMessage.result[u"sessions"].isArray()) {
                        sessions = parsedMessage.result[u"sessions"].toArray();
                    } else {
                        // Some servers might return the sessions array directly as the result
                        // But result is a QJsonObject, not an array, so this shouldn't happen
                        // Just emit empty array
                        qCDebug(ACPCLIENT) << "Received session list response without sessions array";
                    }

                    if (!sessions.isEmpty()) {
                        qCDebug(ACPCLIENT) << "Received session list with" << sessions.size() << "sessions";
                    }
                    Q_EMIT sessionListReceived(sessions);
                    m_pendingRequests.erase(it);
                    return;
                }
            }
        }
    }

    // Check for session update notifications
    ACP::SessionUpdateNotification update;
    if (ACP::ACPProtocol::parseSessionUpdate(message, update)) {
        handleSessionUpdate(message);
        return;
    }

    // Check for progress notifications
    ACP::ProgressNotification progress;
    if (ACP::ACPProtocol::parseProgressNotification(message, progress)) {
        handleProgressNotification(message);
        return;
    }

    // Forward the message to interested parties
    Q_EMIT messageReceived(message);
}

void ACPClientServerManager::handlePermissionRequest(const QJsonDocument &doc)
{
    QJsonObject obj = doc.object();

    if (!obj.contains(u"params") || !obj[u"params"].isObject() || !obj.contains(u"id")) {
        qCWarning(ACPCLIENT) << "Invalid permission request format";
        return;
    }

    qint64 requestId = obj[u"id"].toInteger();
    QJsonObject params = obj[u"params"].toObject();

    if (!params.contains(u"toolCall") || !params[u"toolCall"].isObject() || !params.contains(u"options") || !params[u"options"].isArray()) {
        qCWarning(ACPCLIENT) << "Invalid permission request parameters";
        return;
    }

    QJsonObject toolCall = params[u"toolCall"].toObject();
    QJsonArray options = params[u"options"].toArray();

    qCDebug(ACPCLIENT) << "Permission requested for tool call:" << toolCall[u"toolCallId"].toString() << "with" << options.size() << "options";

    Q_EMIT permissionRequested(requestId, toolCall, options);
}

void ACPClientServerManager::handleSessionUpdate(const QJsonDocument &doc)
{
    qCDebug(ACPCLIENT) << "Handling session update:" << doc.toJson();

    QJsonObject obj = doc.object();
    if (!obj.contains(u"params") || !obj[u"params"].isObject()) {
        Q_EMIT messageReceived(doc);
        return;
    }

    QJsonObject params = obj[u"params"].toObject();
    QString sessionId = params[u"sessionId"].toString();

    if (!params.contains(u"update") || !params[u"update"].isObject()) {
        Q_EMIT messageReceived(doc);
        return;
    }

    QJsonObject update = params[u"update"].toObject();

    // Emit the session update signal
    Q_EMIT sessionUpdateReceived(sessionId, update);

    // Also emit the message for backward compatibility
    Q_EMIT messageReceived(doc);
}

void ACPClientServerManager::handleProgressNotification(const QJsonDocument &doc)
{
    qCDebug(ACPCLIENT) << "Handling progress notification:" << doc.toJson();
    // For now, just emit the message
    Q_EMIT messageReceived(doc);
}

// Static method to create a server manager
static ACPClientServerManager *s_instance = nullptr;

ACPClientServerManager *ACPClientServerManager::new_(ACPClientPlugin *plugin, QObject *parent)
{
    if (!s_instance) {
        s_instance = new ACPClientServerManager(plugin, parent);
    }
    return s_instance;
}

void ACPClientServerManager::showMessage(const QString &msg, KTextEditor::Message::MessageType level)
{
    // inform interested view(er) which will decide how/where to show
    Q_EMIT m_plugin->showMessage(level, msg);
}
