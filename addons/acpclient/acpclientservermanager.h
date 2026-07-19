/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QObject>

#include "acpclientserver.h"

#include <map>
#include <memory>

class ACPClientPlugin;

class ACPClientServerManager : public QObject
{
    Q_OBJECT

public:
    explicit ACPClientServerManager(ACPClientPlugin *plugin, QObject *parent = nullptr);
    ~ACPClientServerManager() override;

    // Create a new server connection
    ACPClientServer *createServer(const ACPClientServer::ServerInfo &info);

    // Load servers from default configuration
    void loadDefaultServers();

    // Get all connected servers
    QList<ACPClientServer *> servers() const;

    // Get server by name
    ACPClientServer *server(const QString &name) const;

    // Remove and delete a server
    void removeServer(ACPClientServer *server);

    // Get the active server (for now, just the first one)
    ACPClientServer *activeServer() const;

    // Set active server by name
    void setActiveServer(const QString &name);

    // Start all auto-start servers
    void startAutoStartServers();

    // Create a new session on the active server
    QString createSession();

    // Load an existing session
    QString loadSession(const QString &sessionId);

    // Resume an existing session
    QString resumeSession(const QString &sessionId);

    // Close a session
    void closeSession(const QString &sessionId);

    // Send a prompt to a session
    void sendPrompt(const QString &sessionId, const QString &message);

    // List sessions
    void listSessions();

    // Delete a session
    void deleteSession(const QString &sessionId);

    // Get tools from active server
    void listTools();

    // Call a tool
    void callTool(const QString &toolId, const QJsonObject &arguments);

Q_SIGNALS:
    void serverAdded(ACPClientServer *server);
    void serverRemoved(ACPClientServer *server);
    void activeServerChanged(ACPClientServer *server);
    void sessionCreated(const QString &sessionId);
    void sessionLoaded(const QString &sessionId);
    void sessionResumed(const QString &sessionId);
    void sessionClosed(const QString &sessionId);
    void sessionListReceived(const QJsonArray &sessions);
    void sessionDeleted(const QString &sessionId);
    void toolsReceived(const QJsonArray &tools);
    void toolCallCompleted(qint64 callId, const QJsonObject &result);
    void messageReceived(const QJsonDocument &message);
    void errorOccurred(const QString &error);

    // Permission request signal
    void permissionRequested(qint64 requestId, const QJsonObject &toolCall, const QJsonArray &options);

    // Session update notification signal
    void sessionUpdateReceived(const QString &sessionId, const QJsonObject &update);

private Q_SLOTS:
    void onServerInitialized(const ACP::InitializeResult &result);
    void onServerDisconnected();
    void onServerError(const QString &error);
    void onServerMessageReceived(const QJsonDocument &message);

private:
    void handleSessionUpdate(const QJsonDocument &doc);
    void handleProgressNotification(const QJsonDocument &doc);
    void handlePermissionRequest(const QJsonDocument &doc);

    ACPClientPlugin *m_plugin;
    std::vector<std::unique_ptr<ACPClientServer>> m_servers;
    ACPClientServer *m_activeServer = nullptr;
    QString m_activeServerName;

    // Track pending session creation requests
    std::map<qint64, qint64> m_pendingSessionRequests;

public:
    static ACPClientServerManager *new_(ACPClientPlugin *plugin, QObject *parent = nullptr);
};
