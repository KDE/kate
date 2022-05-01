/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "socketbus.h"
#include <QDebug>

#include "settings.h"

namespace dap
{
SocketBus::SocketBus(QObject *parent)
    : Bus(parent)
{
    connect(&socket, &QTcpSocket::readyRead, this, &Bus::readyRead);
    connect(&socket, &QTcpSocket::stateChanged, this, &SocketBus::onStateChanged);
}

QByteArray SocketBus::read()
{
    return socket.readAll();
}

quint16 SocketBus::write(const QByteArray &data)
{
    return socket.write(data);
}

bool SocketBus::start(const settings::BusSettings &configuration)
{
    if (!configuration.hasConnection())
        return false;

    socket.connectToHost(configuration.connection->host, configuration.connection->port);

    return true;
}

void SocketBus::close()
{
    socket.close();
    setState(State::Closed);
}

void SocketBus::onStateChanged(QAbstractSocket::SocketState socketState)
{
    // any
    //   connectedstate
    // !connectestate

    if (socketState == QAbstractSocket::SocketState::ConnectedState) {
        setState(State::Running);
    } else {
        const bool errorOccurred = socket.error() != QAbstractSocket::SocketError::UnknownSocketError;
        if (errorOccurred) {
            qWarning() << "Socket Error: " << socket.errorString();
            Q_EMIT error(socket.errorString());
        }
        if (errorOccurred || (state() == State::Running)) {
            setState(State::Closed);
        }
    }
}

}
