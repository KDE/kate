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
#include <QSet>

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
                        Q_EMIT errorOccurred(i18n("Failed to parse server configuration '%1': no JSON object", configPath));
                    }
                } else {
                    Q_EMIT errorOccurred(i18n("Failed to parse server configuration '%1': %2", configPath, error.errorString()));
                }
            }
        } else {
            Q_EMIT errorOccurred(i18n("Failed to read server configuration: %1", configPath));
        }
    }

    // Create the servers for the config
    // Ensure we only have servers that are still in the config

    // Build a set of server names from the config
    QSet<QString> configServerNames;
    if (m_serverConfig.contains(u"servers") && m_serverConfig[u"servers"].isObject()) {
        QJsonObject servers = m_serverConfig[u"servers"].toObject();
        for (auto it = servers.constBegin(); it != servers.constEnd(); ++it) {
            configServerNames.insert(it.key());
        }
    }

    // Remove servers that are no longer in the config
    for (auto it = m_servers.begin(); it != m_servers.end();) {
        const QString &serverName = it->first;
        if (!configServerNames.contains(serverName)) {
            // Server is no longer in config, remove it
            ACPClientServer *server = it->second.get();
            server->stop();

            if (m_activeServer == server) {
                m_activeServer = nullptr;
                m_activeServerName.clear();
                Q_EMIT activeServerChanged(nullptr);
            }

            Q_EMIT serverRemoved(server);
            it = m_servers.erase(it);
        } else {
            ++it;
        }
    }

    // Create servers from the config that don't already exist
    if (m_serverConfig.contains(u"servers") && m_serverConfig[u"servers"].isObject()) {
        QJsonObject servers = m_serverConfig[u"servers"].toObject();
        for (auto it = servers.constBegin(); it != servers.constEnd(); ++it) {
            const QString &serverName = it.key();

            // Skip if server already exists
            if (m_servers.find(serverName) != m_servers.end()) {
                continue;
            }

            const QJsonObject &serverObj = it.value().toObject();
            ACPClientServer::ServerInfo info;

            info.name = serverName;
            info.version = serverObj[u"version"].toString();
            info.command = serverObj[u"command"].toString();

            if (serverObj.contains(u"arguments") && serverObj[u"arguments"].isArray()) {
                const QJsonArray args = serverObj[u"arguments"].toArray();
                for (const QJsonValue &arg : args) {
                    info.arguments.append(arg.toString());
                }
            }

            // Create the server but don't auto-start here (handled by createServer)
            createServer(info);
        }
    }
}

ACPClientServerManager::~ACPClientServerManager()
{
    // Clean up all servers
    for (auto &serverPair : m_servers) {
        serverPair.second->stop();
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

    // Forward the traffic and the process output for logging, with the server
    // it belongs to, which the views have no other way of knowing
    connect(serverPtr, &ACPClientServer::messageReceived, this, [this, serverPtr](const QJsonDocument &message) {
        Q_EMIT serverMessageLogged(serverPtr, message, false);
    });
    connect(serverPtr, &ACPClientServer::messageSent, this, [this, serverPtr](const QJsonDocument &message) {
        Q_EMIT serverMessageLogged(serverPtr, message, true);
    });
    connect(serverPtr, &ACPClientServer::logMessage, this, [this, serverPtr](const QString &text, bool isError) {
        Q_EMIT serverTextLogged(serverPtr, text, isError);
    });

    m_servers[info.name] = std::move(server);

    Q_EMIT serverAdded(serverPtr);

    return serverPtr;
}

QList<ACPClientServer *> ACPClientServerManager::servers() const
{
    QList<ACPClientServer *> result;
    for (const auto &serverPair : m_servers) {
        result.append(serverPair.second.get());
    }
    return result;
}

ACPClientServer *ACPClientServerManager::server(const QString &name) const
{
    auto it = m_servers.find(name);
    if (it != m_servers.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ACPClientServerManager::removeServer(ACPClientServer *server)
{
    for (auto it = m_servers.begin(); it != m_servers.end(); ++it) {
        if (it->second.get() == server) {
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
    for (const auto &serverPair : m_servers) {
        if (serverPair.second->state() == ACPClientServer::ServerState::Initialized) {
            return serverPair.second.get();
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

bool ACPClientServerManager::supportsCloseSession() const
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        return false;
    }
    return server->capabilities().sessionCapabilities.close;
}

bool ACPClientServerManager::supportsDeleteSession() const
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        return false;
    }
    return server->capabilities().sessionCapabilities.deleteSession;
}

bool ACPClientServerManager::supportsSetMode() const
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        return false;
    }
    // session/set_mode is always available if the agent supports it
    // For now, we assume it's available if the agent supports sessions
    return server->capabilities().supportsSessions;
}

bool ACPClientServerManager::supportsLogout() const
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        return false;
    }
    return server->capabilities().auth.logout;
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

    // Track the request type so the response can create the session
    m_pendingRequests[requestId] = PendingRequestType::SessionNew;

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

    // Track the request and remember which session it refers to
    m_pendingRequests[requestId] = PendingRequestType::SessionLoad;
    m_pendingSessionIds[requestId] = sessionId;

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

    // Track the request and remember which session it refers to
    m_pendingRequests[requestId] = PendingRequestType::SessionResume;
    m_pendingSessionIds[requestId] = sessionId;

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

    // The response ends the prompt turn, so it has to be recognized
    m_pendingRequests[requestId] = PendingRequestType::SessionPrompt;
    m_pendingSessionIds[requestId] = sessionId;

    server->sendMessage(request);

    Q_EMIT promptStarted(sessionId);
}

void ACPClientServerManager::cancelPrompt(const QString &sessionId)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    qCDebug(ACPCLIENT) << "Cancelling the prompt turn of session" << sessionId;

    // A notification, so the turn only ends once the agent answers the prompt
    // request it is still holding
    server->sendMessage(ACP::ACPProtocol::createSessionCancelNotification(sessionId));
}

void ACPClientServerManager::setSessionMode(const QString &sessionId, const QString &modeId)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionSetModeRequest(sessionId, modeId, requestId);

    m_pendingRequests[requestId] = PendingRequestType::SessionSetMode;
    m_pendingSessionIds[requestId] = sessionId;

    server->sendMessage(request);
}

void ACPClientServerManager::setSessionModel(const QString &sessionId, const QString &modelId)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionSetModelRequest(sessionId, modelId, requestId);

    m_pendingRequests[requestId] = PendingRequestType::SessionSetModel;
    m_pendingSessionIds[requestId] = sessionId;

    server->sendMessage(request);
}

void ACPClientServerManager::setSessionConfigOption(const QString &sessionId, const QString &configId, const QString &value)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    qint64 requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionSetConfigOptionRequest(sessionId, configId, value, requestId);

    m_pendingRequests[requestId] = PendingRequestType::SessionSetConfigOption;
    m_pendingSessionIds[requestId] = sessionId;

    server->sendMessage(request);
}

void ACPClientServerManager::listSessions()
{
    // no active server => no sessions!
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT sessionListReceived({});
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
        // Check for responses we are waiting for by matching the request ID
        if (parsedMessage.isResponse && parsedMessage.id != 0) {
            auto requestIt = m_pendingRequests.find(parsedMessage.id);
            if (requestIt != m_pendingRequests.end()) {
                const PendingRequestType requestType = requestIt->second;
                const QJsonObject result = parsedMessage.result;

                // Session IDs are remembered for requests whose response does not echo them
                QString requestedSessionId;
                auto sessionIdIt = m_pendingSessionIds.find(parsedMessage.id);
                if (sessionIdIt != m_pendingSessionIds.end()) {
                    requestedSessionId = sessionIdIt->second;
                    m_pendingSessionIds.erase(sessionIdIt);
                }

                // An error response carries no result, so it has to be told
                // apart from a successful one before reading the result
                const bool isErrorResponse = obj.contains(u"error");

                switch (requestType) {
                case PendingRequestType::SessionPrompt: {
                    m_pendingRequests.erase(requestIt);
                    if (isErrorResponse) {
                        Q_EMIT promptFailed(requestedSessionId, parsedMessage.error.message);
                    } else {
                        Q_EMIT promptFinished(requestedSessionId, result[u"stopReason"].toString());
                    }
                    return;
                }
                case PendingRequestType::SessionNew: {
                    m_pendingRequests.erase(requestIt);
                    const QString sessionId = result[u"sessionId"].toString();
                    if (!sessionId.isEmpty()) {
                        qCDebug(ACPCLIENT) << "Session response received for ID:" << sessionId;
                        Q_EMIT sessionCreated(sessionId);
                        emitSessionStateFromResult(sessionId, result);
                    }
                    return;
                }
                case PendingRequestType::SessionLoad:
                case PendingRequestType::SessionResume: {
                    m_pendingRequests.erase(requestIt);
                    QString sessionId = result[u"sessionId"].toString();
                    if (sessionId.isEmpty()) {
                        sessionId = requestedSessionId;
                    }
                    if (!sessionId.isEmpty()) {
                        if (requestType == PendingRequestType::SessionLoad) {
                            Q_EMIT sessionLoaded(sessionId);
                        } else {
                            Q_EMIT sessionResumed(sessionId);
                        }
                        emitSessionStateFromResult(sessionId, result);
                    }
                    return;
                }
                case PendingRequestType::SessionList: {
                    qCDebug(ACPCLIENT) << "Received session/list response:" << result;

                    // Try to extract sessions from different possible locations
                    QJsonArray sessions;
                    if (result.contains(u"sessions") && result[u"sessions"].isArray()) {
                        sessions = result[u"sessions"].toArray();
                    } else {
                        qCDebug(ACPCLIENT) << "Received session list response without sessions array";
                    }

                    if (!sessions.isEmpty()) {
                        qCDebug(ACPCLIENT) << "Received session list with" << sessions.size() << "sessions";
                    }
                    Q_EMIT sessionListReceived(sessions);
                    m_pendingRequests.erase(requestIt);
                    return;
                }
                case PendingRequestType::SessionSetConfigOption: {
                    m_pendingRequests.erase(requestIt);
                    if (result.contains(u"configOptions") && result[u"configOptions"].isArray()) {
                        QList<ACP::SessionConfigOption> configOptions;
                        if (ACP::ACPProtocol::parseSessionConfigOptions(result[u"configOptions"].toArray(), configOptions) && !requestedSessionId.isEmpty()) {
                            Q_EMIT sessionConfigOptionsChanged(requestedSessionId, configOptions);
                        }
                    }
                    return;
                }
                case PendingRequestType::SessionSetMode:
                case PendingRequestType::SessionSetModel: {
                    m_pendingRequests.erase(requestIt);
                    if (!requestedSessionId.isEmpty()) {
                        emitSessionStateFromResult(requestedSessionId, result);
                    }
                    return;
                }
                default:
                    break;
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

    // Store the raw ID value (can be integer or string)
    QJsonValue idValue = obj[u"id"];

    // Validate the ID type
    if (!idValue.isDouble() && !idValue.isString()) {
        qCWarning(ACPCLIENT) << "Invalid permission request ID type";
        return;
    }

    if (idValue.isString()) {
        // Log non-numeric string IDs
        bool ok;
        idValue.toString().toLongLong(&ok);
        if (!ok) {
            qCDebug(ACPCLIENT) << "Permission request has non-numeric string ID:" << idValue.toString();
        }
    }

    QJsonObject params = obj[u"params"].toObject();

    if (!params.contains(u"toolCall") || !params[u"toolCall"].isObject() || !params.contains(u"options") || !params[u"options"].isArray()) {
        qCWarning(ACPCLIENT) << "Invalid permission request parameters";
        return;
    }

    QJsonObject toolCall = params[u"toolCall"].toObject();
    QJsonArray options = params[u"options"].toArray();

    qCDebug(ACPCLIENT) << "Permission requested for tool call:" << toolCall[u"toolCallId"].toString() << "with" << options.size() << "options";

    // Emit the signal with the raw ID (can be integer or string)
    Q_EMIT permissionRequested(idValue, toolCall, options);
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

void ACPClientServerManager::emitSessionStateFromResult(const QString &sessionId, const QJsonObject &result)
{
    if (sessionId.isEmpty()) {
        return;
    }

    if (result.contains(u"modes") && result[u"modes"].isObject()) {
        ACP::SessionModeState modes;
        if (ACP::ACPProtocol::parseSessionModes(result[u"modes"].toObject(), modes)) {
            Q_EMIT sessionModesChanged(sessionId, modes);
        }
    }

    if (result.contains(u"models") && result[u"models"].isObject()) {
        ACP::SessionModelState models;
        if (ACP::ACPProtocol::parseSessionModels(result[u"models"].toObject(), models)) {
            Q_EMIT sessionModelsChanged(sessionId, models);
        }
    }

    if (result.contains(u"configOptions") && result[u"configOptions"].isArray()) {
        QList<ACP::SessionConfigOption> configOptions;
        if (ACP::ACPProtocol::parseSessionConfigOptions(result[u"configOptions"].toArray(), configOptions)) {
            Q_EMIT sessionConfigOptionsChanged(sessionId, configOptions);
        }
    }
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
