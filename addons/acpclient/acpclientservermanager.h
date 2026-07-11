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
    void sessionListReceived(const QJsonArray &sessions);
    void sessionDeleted(const QString &sessionId);
    void toolsReceived(const QJsonArray &tools);
    void toolCallCompleted(const QString &callId, const QJsonObject &result);
    void messageReceived(const QJsonDocument &message);
    void errorOccurred(const QString &error);

private Q_SLOTS:
    void onServerInitialized(const ACP::InitializeResult &result);
    void onServerDisconnected();
    void onServerError(const QString &error);
    void onServerMessageReceived(const QJsonDocument &message);

private:
    void handleSessionUpdate(const QJsonDocument &doc);
    void handleProgressNotification(const QJsonDocument &doc);

    ACPClientPlugin *m_plugin;
    std::vector<std::unique_ptr<ACPClientServer>> m_servers;
    ACPClientServer *m_activeServer = nullptr;
    QString m_activeServerName;

public:
    static ACPClientServerManager *new_(ACPClientPlugin *plugin, QObject *parent = nullptr);
};
