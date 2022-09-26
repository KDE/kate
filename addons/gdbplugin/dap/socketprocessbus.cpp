/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "settings.h"
#include <QTextCodec>
#include <QTimer>
#include <memory>

#include "dapclient_debug.h"
#include "socketprocessbus.h"

constexpr int TIMEOUT = 1000;

namespace dap
{
SocketProcessBus::SocketProcessBus(QObject *parent)
    : Bus(parent)
{
    connect(&socket, &QTcpSocket::readyRead, this, &Bus::readyRead);
    connect(&socket, &QTcpSocket::stateChanged, this, &SocketProcessBus::onSocketStateChanged);
    connect(&process, &QProcess::stateChanged, this, &SocketProcessBus::onProcessStateChanged);

    connect(&process, &QProcess::readyReadStandardError, this, &SocketProcessBus::readError);
    connect(&process, &QProcess::readyReadStandardOutput, this, &SocketProcessBus::readOutput);
}

SocketProcessBus::~SocketProcessBus()
{
    blockSignals(true);
    if (socket.state() == QTcpSocket::SocketState::ConnectedState) {
        socket.close();
    }
    bool finished = process.state() == QProcess::NotRunning;
    if (!finished) {
        process.terminate();
        finished = process.waitForFinished(500);
    }
    if (!finished) {
        process.kill();
        process.waitForFinished(300);
    }
}

QByteArray SocketProcessBus::read()
{
    return socket.readAll();
}

quint16 SocketProcessBus::write(const QByteArray &data)
{
    return socket.write(data);
}

bool SocketProcessBus::start(const settings::BusSettings &configuration)
{
    if (!configuration.hasConnection() || !configuration.hasCommand())
        return false;

    const auto &connection = configuration.connection.value();

    m_connectionHandler.reset();
    m_connectionHandler = [this, connection]() {
        this->socket.connectToHost(connection.host, connection.port);
    };

    configuration.command->start(process);

    return true;
}

void SocketProcessBus::closeResources()
{
    qCDebug(DAPCLIENT) << "[BUS] closing resources";
    if (socket.state() == QTcpSocket::SocketState::ConnectedState) {
        socket.close();
    }
    if (process.state() != QProcess::NotRunning) {
        if (m_tryClose == None) {
            m_tryClose = Terminate;
            process.terminate();
        } else if (m_tryClose == None) {
            m_tryClose = Kill;
            process.kill();
        } else {
            process.waitForFinished(500);
        }
    }
}

void SocketProcessBus::onSocketStateChanged(const QAbstractSocket::SocketState &state)
{
    qCDebug(DAPCLIENT) << "SOCKET STATE " << state;

    const bool socketError = socket.error() != QAbstractSocket::SocketError::UnknownSocketError;
    if (socketError)
        qCDebug(DAPCLIENT) << socket.errorString();

    if (state == QTcpSocket::SocketState::ConnectedState) {
        m_connectionHandler.reset();
        setState(State::Running);
        return;
    }
    if (socketError) {
        Q_EMIT error(process.errorString());
        close();
    }
}

void SocketProcessBus::onProcessStateChanged(const QProcess::ProcessState &state)
{
    qCDebug(DAPCLIENT) << "PROCESS STATE " << state;

    const bool processError = process.error() != QProcess::ProcessError::UnknownError;
    if (processError) {
        Q_EMIT error(process.errorString());
        close();
        return;
    }
    switch (state) {
    case QProcess::ProcessState::Running:
        QTimer::singleShot(TIMEOUT, this, &SocketProcessBus::connectSocket);
        break;
    case QProcess::ProcessState::NotRunning:
        close();
        break;
    default:;
    }
}

void SocketProcessBus::connectSocket()
{
    qCDebug(DAPCLIENT) << "connect to socket INIT";
    if (!m_connectionHandler)
        return;
    qCDebug(DAPCLIENT) << "connect to socket with handler";
    (*m_connectionHandler)();
}

void SocketProcessBus::close()
{
    closeResources();
    setState(State::Closed);
}

void SocketProcessBus::readError()
{
    const auto &message = process.readAllStandardError();
    // process' standard error
    qCDebug(DAPCLIENT) << "[BUS] STDERR << " << message;

    Q_EMIT serverOutput(QTextCodec::codecForLocale()->toUnicode(message));
}

void SocketProcessBus::readOutput()
{
    const auto &message = process.readAllStandardOutput();
    qCDebug(DAPCLIENT) << "[BUS] STDOUT << " << message;

    Q_EMIT processOutput(QTextCodec::codecForLocale()->toUnicode(message));
}

}
