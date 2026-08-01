/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "acpclientserver.h"
#include "acpclient_debug.h"
#include "acpclientservermanager.h"

#include <KAboutData>
#include <QIODevice>
#include <QJsonParseError>
#include <QProcess>

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
    setupProcessConnection();
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

    setState(ServerState::Disconnected);
}

void ACPClientServer::sendMessage(const QJsonDocument &message)
{
    QByteArray data = message.toJson(QJsonDocument::Compact);
    qCDebug(ACPCLIENT) << "Sending message:" << data;

    if (m_process && m_process->isWritable()) {
        m_process->write(data + "\n");
    }
}

void ACPClientServer::setupProcessConnection()
{
    qCDebug(ACPCLIENT) << "Setting up process connection for:" << m_info.command << m_info.arguments;

    m_process = std::make_unique<QProcess>(this);
    m_process->setProgram(m_info.command);
    m_process->setArguments(m_info.arguments);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    // Only read from stdout for JSON-RPC messages
    connect(m_process.get(), &QProcess::readyReadStandardOutput, this, &ACPClientServer::onProcessReadyRead);
    // Read stderr for error messages
    connect(m_process.get(), &QProcess::readyReadStandardError, this, &ACPClientServer::onProcessErrorOutput);
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

void ACPClientServer::initializeServer()
{
    if (m_state != ServerState::Connected) {
        return;
    }

    setState(ServerState::Initializing);

    ACP::InitializeParams params;
    params.protocolVersion = ACP::PROTOCOL_VERSION_INT;
    params.clientInfo.name = QStringLiteral("kate");
    params.clientInfo.title = QStringLiteral("Kate ACP Client");
    params.clientInfo.version = KAboutData::applicationData().version();

    // Set client capabilities
    params.clientCapabilities.fs.readTextFile = false;
    params.clientCapabilities.fs.writeTextFile = false;
    params.clientCapabilities.terminal = false;
    params.clientCapabilities.sessionConfigOptionsBoolean.supported = false;

    qint64 requestId = ACP::ACPProtocol::generateRequestId();
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

    // read the process output line wise
    while (m_process->canReadLine()) {
        const QByteArray line = m_process->readLine();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            handleMessage(doc);
        } else {
            qCWarning(ACPCLIENT) << "JSON parse error:" << parseError.errorString();
        }
    }
}

void ACPClientServer::onProcessErrorOutput()
{
    if (m_process) {
        QByteArray errorData = m_process->readAllStandardError();
        if (!errorData.isEmpty()) {
            qCWarning(ACPCLIENT) << "ACP server stderr:" << errorData;
            Q_EMIT errorOccurred(QString::fromUtf8(errorData));
        }
    }
}

void ACPClientServer::onProcessError(QProcess::ProcessError error)
{
    qCWarning(ACPCLIENT) << "Process error:" << error << ":" << m_process->errorString();
    setState(ServerState::Error);
    Q_EMIT errorOccurred(m_process->errorString());
}

void ACPClientServer::onProcessFinished(int exitCode, QProcess::ExitStatus)
{
    qCDebug(ACPCLIENT) << "Process finished with exit code:" << exitCode;
    setState(ServerState::Disconnected);
    Q_EMIT disconnected();
}

void ACPClientServer::handleMessage(const QJsonDocument &doc)
{
    qCDebug(ACPCLIENT) << "Handling message:\n" << doc.toJson(QJsonDocument::Indented) << "\n\n";

    ACP::ACPMessage message;
    if (ACP::ACPProtocol::parseMessage(doc, message)) {
        // Check if it's a response to a pending request
        if (message.isResponse && message.id != 0) {
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
