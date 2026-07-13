/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpclientservermanager.h"
#include "acpclient_debug.h"
#include "acpclientplugin.h"
#include "acpclientprotocol.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonObject>

ACPClientServerManager::ACPClientServerManager(ACPClientPlugin *plugin, QObject *parent)
    : QObject(parent)
    , m_plugin(plugin)
{
    qCDebug(ACPCLIENT) << "ACPClientServerManager created";

    // Load default servers on creation
    loadDefaultServers();

    // Start auto-start servers
    startAutoStartServers();
}

void ACPClientServerManager::loadDefaultServers()
{
    qCDebug(ACPCLIENT) << "Loading default ACP servers";

    // Load from shipped settings.json
    QString settingsPath = QStringLiteral(":/kateacpclient/settings.json");
    QFile settingsFile(settingsPath);

    if (settingsFile.exists() && settingsFile.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(settingsFile.readAll(), &parseError);

        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains(u"servers") && obj[u"servers"].isArray()) {
                QJsonArray servers = obj[u"servers"].toArray();
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

                        info.autoStart = serverObj[u"auto_start"].toBool();

                        if (serverObj.contains(u"metadata") && serverObj[u"metadata"].isObject()) {
                            info.metadata = serverObj[u"metadata"].toObject();
                        }

                        // Create the server but don't auto-start here (handled by createServer)
                        createServer(info);
                    }
                }
            }
        }
        settingsFile.close();
    }

    // If no servers were loaded, add vibe-acp as default
    if (m_servers.empty()) {
        qCDebug(ACPCLIENT) << "No servers in config, adding default vibe-acp";
        ACPClientServer::ServerInfo vibeServer;
        vibeServer.name = QStringLiteral("Mistral Vibe (vibe-acp)");
        vibeServer.version = QStringLiteral("1.0");
        vibeServer.command = QStringLiteral("vibe-acp");
        vibeServer.autoStart = true;
        createServer(vibeServer);
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
    connect(serverPtr, &ACPClientServer::stateChanged, this, [this, serverPtr, info](ACPClientServer::ServerState state) {
        if (state == ACPClientServer::ServerState::Initialized) {
            if (!m_activeServer && info.autoStart) {
                setActiveServer(serverPtr->info().name);
            }
        }
    });

    m_servers.push_back(std::move(server));

    Q_EMIT serverAdded(serverPtr);

    // Start the server if it should auto-start
    if (info.autoStart) {
        serverPtr->start();
    }

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

void ACPClientServerManager::startAutoStartServers()
{
    for (auto &server : m_servers) {
        if (server->info().autoStart && server->state() == ACPClientServer::ServerState::Disconnected) {
            server->start();
        }
    }
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
    QString requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionNewRequest(params, requestId);

    // Track the request for response - store the requestId and emit signal when response arrives
    // We use a map to track pending session creations
    m_pendingSessionRequests[requestId] = requestId;

    server->sendMessage(request);

    // Return empty for now - the actual session ID will come via signal
    return QString();
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
    params.message = message;

    QString requestId = ACP::ACPProtocol::generateRequestId();
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

    QString requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionListRequest(requestId);

    // Track the request
    auto callback = [this](const QJsonDocument &response) {
        if (response.isObject()) {
            QJsonObject obj = response.object();
            if (obj.contains(u"result") && obj[u"result"].isArray()) {
                Q_EMIT sessionListReceived(obj[u"result"].toArray());
            }
        }
    };

    // For now, we need to implement request tracking in the server
    server->sendMessage(request);
}

void ACPClientServerManager::deleteSession(const QString &sessionId)
{
    ACPClientServer *server = activeServer();
    if (!server || server->state() != ACPClientServer::ServerState::Initialized) {
        Q_EMIT errorOccurred(tr("No active ACP server available"));
        return;
    }

    QString requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createSessionDeleteRequest(sessionId, requestId);

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

    QString requestId = ACP::ACPProtocol::generateRequestId();
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

    QString requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createToolsCallRequest(toolId, arguments, requestId);

    server->sendMessage(request);
    Q_EMIT toolCallCompleted(requestId, QJsonObject());
}

void ACPClientServerManager::onServerInitialized(const ACP::InitializeResult &result)
{
    ACPClientServer *server = qobject_cast<ACPClientServer *>(sender());
    if (server) {
        qCDebug(ACPCLIENT) << "Server initialized:" << server->info().name << "Agent:" << result.agentName << "Version:" << result.agentVersion
                           << "Protocol:" << result.protocolVersion;
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
    ACPClientServer *server = qobject_cast<ACPClientServer *>(sender());
    if (server) {
        qCDebug(ACPCLIENT) << "Message received from" << server->info().name << ":" << message.toJson();
    }

    ACP::ACPMessage parsedMessage;
    if (ACP::ACPProtocol::parseMessage(message, parsedMessage)) {
        // Check for session/new response by matching the request ID
        if (parsedMessage.isResponse && !parsedMessage.id.isEmpty()) {
            // Check if this is a response to a session/new request
            if (m_pendingSessionRequests.find(parsedMessage.id) != m_pendingSessionRequests.end()) {
                QJsonObject result = parsedMessage.result;
                if (result.contains(u"sessionId")) {
                    QString sessionId = result[u"sessionId"].toString();
                    qCDebug(ACPCLIENT) << "Session created with ID:" << sessionId;
                    m_pendingSessionRequests.erase(parsedMessage.id);
                    Q_EMIT sessionCreated(sessionId);
                    return;
                }
                // Remove from pending even if there was an error
                m_pendingSessionRequests.erase(parsedMessage.id);
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

void ACPClientServerManager::handleSessionUpdate(const QJsonDocument &doc)
{
    qCDebug(ACPCLIENT) << "Handling session update:" << doc.toJson();
    // For now, just emit the message
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
