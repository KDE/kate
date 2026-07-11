/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#include "acpclientserver.h"
#include "acpclient_debug.h"
#include "acpclientservermanager.h"

#include <QIODevice>
#include <QJsonParseError>
#include <QProcess>
#include <QTcpSocket>
#include <QWebSocket>

ACPClientServer::ACPClientServer(const ServerInfo &info, ACPClientServerManager *manager, QObject *parent)
    : QObject(parent)
    , m_info(info)
    , m_manager(manager)
    , m_state(ServerState::Disconnected)
{
    qCDebug(ACPCLIENT) << "ACPClientServer created for" << info.name;
}

ACPClientServer::~ACPClientServer()
{
    stop();
    qCDebug(ACPCLIENT) << "ACPClientServer destroyed for" << m_info.name;
}

void ACPClientServer::start()
{
    if (m_state != ServerState::Disconnected) {
        qCWarning(ACPCLIENT) << "Server already starting or connected:" << m_info.name;
        return;
    }

    setState(ServerState::Connecting);

    switch (m_info.connectionType) {
    case ConnectionType::StdIO:
        setupProcessConnection();
        break;
    case ConnectionType::WebSocket:
        setupWebSocketConnection();
        break;
    case ConnectionType::TcpSocket:
        setupTcpSocketConnection();
        break;
    }
}

void ACPClientServer::stop()
{
    qCDebug(ACPCLIENT) << "Stopping ACP server:" << m_info.name;

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
        }
    }

    if (m_webSocket && m_webSocket->isValid()) {
        m_webSocket->close();
    }

    if (m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        m_tcpSocket->disconnectFromHost();
    }

    setState(ServerState::Disconnected);
}

void ACPClientServer::sendMessage(const QJsonDocument &message)
{
    QByteArray data = message.toJson(QJsonDocument::Compact);
    qCDebug(ACPCLIENT) << "Sending message:" << data;

    switch (m_info.connectionType) {
    case ConnectionType::StdIO:
        if (m_process && m_process->isWritable()) {
            m_process->write(data + "\n");
            m_process->flush();
        }
        break;
    case ConnectionType::WebSocket:
        if (m_webSocket && m_webSocket->isValid()) {
            m_webSocket->sendBinaryMessage(data);
        }
        break;
    case ConnectionType::TcpSocket:
        if (m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
            m_tcpSocket->write(data + "\n");
            m_tcpSocket->flush();
        }
        break;
    }
}

void ACPClientServer::setupProcessConnection()
{
    qCDebug(ACPCLIENT) << "Setting up process connection for:" << m_info.command << m_info.arguments;

    m_process = std::make_unique<QProcess>(this);
    m_process->setProgram(m_info.command);
    m_process->setArguments(m_info.arguments);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process.get(), &QProcess::readyRead, this, &ACPClientServer::onProcessReadyRead);
    connect(m_process.get(), &QProcess::errorOccurred, this, &ACPClientServer::onProcessError);
    connect(m_process.get(), &QProcess::finished, this, &ACPClientServer::onProcessFinished);

    m_process->start();
    if (!m_process->waitForStarted(5000)) {
        qCWarning(ACPCLIENT) << "Failed to start process:" << m_info.command;
        setState(ServerState::Error);
        Q_EMIT errorOccurred(tr("Failed to start ACP agent process"));
    } else {
        setState(ServerState::Connected);
        // Start initialization
        initializeServer();
    }
}

void ACPClientServer::setupWebSocketConnection()
{
    qCDebug(ACPCLIENT) << "Setting up WebSocket connection to:" << m_info.host << ":" << m_info.port;

    m_webSocket = std::make_unique<QWebSocket>(QString(), QWebSocket::VersionLatest, this);

    connect(m_webSocket.get(), &QWebSocket::connected, this, &ACPClientServer::onWebSocketConnected);
    connect(m_webSocket.get(), &QWebSocket::disconnected, this, &ACPClientServer::onWebSocketDisconnected);
    connect(m_webSocket.get(), &QWebSocket::error, this, &ACPClientServer::onWebSocketError);
    connect(m_webSocket.get(), &QWebSocket::binaryMessageReceived, this, &ACPClientServer::onWebSocketMessageReceived);

    QUrl url;
    url.setScheme("ws");
    url.setHost(m_info.host);
    url.setPort(m_info.port);

    m_webSocket->open(url);
}

void ACPClientServer::setupTcpSocketConnection()
{
    qCDebug(ACPCLIENT) << "Setting up TCP connection to:" << m_info.host << ":" << m_info.port;

    m_tcpSocket = std::make_unique<QTcpSocket>(this);

    connect(m_tcpSocket.get(), &QTcpSocket::connected, this, &ACPClientServer::onTcpSocketConnected);
    connect(m_tcpSocket.get(), &QTcpSocket::disconnected, this, &ACPClientServer::onTcpSocketDisconnected);
    connect(m_tcpSocket.get(), &QTcpSocket::errorOccurred, this, &ACPClientServer::onTcpSocketError);
    connect(m_tcpSocket.get(), &QTcpSocket::readyRead, this, &ACPClientServer::onTcpSocketReadyRead);

    m_tcpSocket->connectToHost(m_info.host, m_info.port);
    if (!m_tcpSocket->waitForConnected(5000)) {
        qCWarning(ACPCLIENT) << "Failed to connect to TCP socket:" << m_info.host << ":" << m_info.port;
        setState(ServerState::Error);
        Q_EMIT errorOccurred(tr("Failed to connect to ACP agent TCP socket"));
    }
}

void ACPClientServer::initializeServer()
{
    if (m_state != ServerState::Connected) {
        return;
    }

    setState(ServerState::Initializing);

    ACP::InitializeParams params;
    params.clientName = QStringLiteral("Kate");
    params.clientVersion = QStringLiteral("26.11.70"); // Will be updated
    params.capabilities = ACP::ClientCapabilities();

    QString requestId = ACP::ACPProtocol::generateRequestId();
    QJsonDocument request = ACP::ACPProtocol::createInitializeRequest(params, requestId);

    // Track the request
    m_pendingRequests[requestId] = [this](const QJsonDocument &response) {
        ACP::InitializeResult result;
        if (ACP::ACPProtocol::parseInitializeResponse(response, result)) {
            m_capabilities = result.capabilities;
            m_protocolVersion = result.protocolVersion;
            setState(ServerState::Initialized);
            Q_EMIT initialized(result);
        } else {
            setState(ServerState::Error);
            Q_EMIT errorOccurred(tr("Failed to parse initialize response"));
        }
    };

    sendMessage(request);
}

void ACPClientServer::onProcessReadyRead()
{
    if (m_process) {
        readFromProcess();
    }
}

void ACPClientServer::readFromProcess()
{
    if (!m_process)
        return;

    QByteArray data = m_process->readAll();
    if (!data.isEmpty()) {
        qCDebug(ACPCLIENT) << "Received data:" << data;
        parseIncomingData(data);
    }
}

void ACPClientServer::onProcessError(QProcess::ProcessError error)
{
    qCWarning(ACPCLIENT) << "Process error:" << error << ":" << m_process->errorString();
    setState(ServerState::Error);
    Q_EMIT errorOccurred(m_process->errorString());
}

void ACPClientServer::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(ACPCLIENT) << "Process finished with exit code:" << exitCode;
    setState(ServerState::Disconnected);
    Q_EMIT disconnected();
}

void ACPClientServer::onWebSocketConnected()
{
    qCDebug(ACPCLIENT) << "WebSocket connected";
    setState(ServerState::Connected);
    initializeServer();
}

void ACPClientServer::onWebSocketDisconnected()
{
    qCDebug(ACPCLIENT) << "WebSocket disconnected";
    setState(ServerState::Disconnected);
    Q_EMIT disconnected();
}

void ACPClientServer::onWebSocketError(QAbstractSocket::SocketError error)
{
    qCWarning(ACPCLIENT) << "WebSocket error:" << error;
    setState(ServerState::Error);
    Q_EMIT errorOccurred(m_webSocket->errorString());
}

void ACPClientServer::onWebSocketMessageReceived(const QByteArray &message)
{
    qCDebug(ACPCLIENT) << "WebSocket message received:" << message;
    parseIncomingData(message);
}

void ACPClientServer::onTcpSocketConnected()
{
    qCDebug(ACPCLIENT) << "TCP socket connected";
    setState(ServerState::Connected);
    initializeServer();
}

void ACPClientServer::onTcpSocketDisconnected()
{
    qCDebug(ACPCLIENT) << "TCP socket disconnected";
    setState(ServerState::Disconnected);
    Q_EMIT disconnected();
}

void ACPClientServer::onTcpSocketError(QAbstractSocket::SocketError error)
{
    qCWarning(ACPCLIENT) << "TCP socket error:" << error;
    setState(ServerState::Error);
    Q_EMIT errorOccurred(m_tcpSocket->errorString());
}

void ACPClientServer::onTcpSocketReadyRead()
{
    if (m_tcpSocket) {
        QByteArray data = m_tcpSocket->readAll();
        if (!data.isEmpty()) {
            qCDebug(ACPCLIENT) << "TCP data received:" << data;
            parseIncomingData(data);
        }
    }
}

void ACPClientServer::parseIncomingData(const QByteArray &data)
{
    // Append to buffer
    m_messageBuffer.append(data);

    // Try to parse JSON documents (delimited by newlines)
    int pos = 0;
    while (pos < m_messageBuffer.size()) {
        int endPos = m_messageBuffer.indexOf('\n', pos);
        if (endPos == -1) {
            break; // Incomplete message
        }

        QByteArray messageData = m_messageBuffer.mid(pos, endPos - pos);
        pos = endPos + 1;

        if (messageData.isEmpty()) {
            continue; // Skip empty lines
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(messageData, &parseError);

        if (parseError.error == QJsonParseError::NoError) {
            handleMessage(doc);
        } else {
            qCWarning(ACPCLIENT) << "JSON parse error:" << parseError.errorString();
        }
    }

    // Remove processed data from buffer
    if (pos > 0) {
        m_messageBuffer = m_messageBuffer.mid(pos);
    }
}

void ACPClientServer::handleMessage(const QJsonDocument &doc)
{
    qCDebug(ACPCLIENT) << "Handling message:" << doc.toJson();

    ACP::ACPMessage message;
    if (ACP::ACPProtocol::parseMessage(doc, message)) {
        // Check if it's a response to a pending request
        if (message.isResponse && !message.id.isEmpty()) {
            auto it = m_pendingRequests.find(message.id);
            if (it != m_pendingRequests.end()) {
                it->second(doc);
                m_pendingRequests.erase(it);
                return;
            }
        }

        // Emit message received signal for notifications and other messages
        Q_EMIT messageReceived(doc);
    } else {
        qCWarning(ACPCLIENT) << "Failed to parse ACP message";
    }
}

void ACPClientServer::setState(ServerState state)
{
    if (m_state != state) {
        m_state = state;
        Q_EMIT stateChanged(state);
    }
}

#include "moc_acpclientserver.cpp"