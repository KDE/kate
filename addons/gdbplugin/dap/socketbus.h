/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef DAP_SOCKETBUS_H
#define DAP_SOCKETBUS_H

#include <QTcpSocket>

#include "bus.h"

namespace dap
{
class SocketBus : public Bus
{
    Q_OBJECT
public:
    explicit SocketBus(QObject *parent = nullptr);
    ~SocketBus() override = default;

    QByteArray read() override;
    quint16 write(const QByteArray &data) override;

    bool start(const settings::BusSettings &configuration) override;
    void close() override;

    QTcpSocket socket;

private:
    void onStateChanged(QAbstractSocket::SocketState socketState);
    void onError(QAbstractSocket::SocketError socketError);
};

}

#endif // DAP_SOCKETBUS_H
