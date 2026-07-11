/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QJsonDocument>
#include <QObject>
#include <QProcess>

#include "acpclientprotocol.h"

#include <map>
#include <memory>

class QWebSocket;
class QTcpSocket;

class ACPClientPlugin;
class ACPClientServerManager;

class ACPClientServer : public QObject
{
    Q_OBJECT

public:
    enum class ConnectionType {
        StdIO, // Standard input/output
        WebSocket, // WebSocket connection
        TcpSocket // TCP socket connection
    };

    enum class ServerState {
        Disconnected,
        Connecting,
        Connected,
        Initializing,
        Initialized,
        Error
    };

    struct ServerInfo {
        QString name;
        QString version;
        QString command;
        QStringList arguments;
        ConnectionType connectionType;
        QString host;
        int port = 0;
        bool autoStart = false;
        QJsonObject metadata;
    };

    explicit ACPClientServer(const ServerInfo &info, ACPClientServerManager *manager, QObject *parent = nullptr);
    ~ACPClientServer() override;

    // Start the connection to the server
    void start();
    void stop();

    // Send a message to the server
    void sendMessage(const QJsonDocument &message);

    // Get current state
    ServerState state() const
    {
        return m_state;
    }
    ServerInfo info() const
    {
        return m_info;
    }

    // Get agent capabilities
    ACP::AgentCapabilities capabilities() const
    {
        return m_capabilities;
    }

    // Get negotiated protocol version
    QString protocolVersion() const
    {
        return m_protocolVersion;
    }

Q_SIGNALS:
    void stateChanged(ACPClientServer::ServerState state);
    void messageReceived(const QJsonDocument &message);
    void errorOccurred(const QString &error);
    void initialized(const ACP::InitializeResult &result);
    void disconnected();

private Q_SLOTS:
    void onProcessReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketError(QAbstractSocket::SocketError error);
    void onWebSocketMessageReceived(const QByteArray &message);
    void onTcpSocketConnected();
    void onTcpSocketDisconnected();
    void onTcpSocketError(QAbstractSocket::SocketError error);
    void onTcpSocketReadyRead();

private:
    void setupProcessConnection();
    void setupWebSocketConnection();
    void setupTcpSocketConnection();
    void readFromProcess();
    void parseIncomingData(const QByteArray &data);
    void handleMessage(const QJsonDocument &doc);

    ServerInfo m_info;
    ServerState m_state = ServerState::Disconnected;
    ACPClientServerManager *m_manager;
    ACP::AgentCapabilities m_capabilities;
    QString m_protocolVersion;

    // Connection objects
    std::unique_ptr<QProcess> m_process;
    std::unique_ptr<QWebSocket> m_webSocket;
    std::unique_ptr<QTcpSocket> m_tcpSocket;

    // Buffer for incomplete messages
    QByteArray m_messageBuffer;

    // Request tracking
    std::map<QString, std::function<void(const QJsonDocument &)>> m_pendingRequests;
};