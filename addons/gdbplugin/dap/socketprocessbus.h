/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef DAP_SOCKETPROCESSBUS_H
#define DAP_SOCKETPROCESSBUS_H

#include <QProcess>
#include <QTcpSocket>
#include <optional>
#include <utility>

#include "bus.h"

namespace dap
{
class SocketProcessBus : public Bus
{
    Q_OBJECT
public:
    explicit SocketProcessBus(QObject *parent = nullptr);
    ~SocketProcessBus() override;

    QByteArray read() override;
    quint16 write(const QByteArray &data) override;

    bool start(const settings::BusSettings &configuration) override;
    void close() override;

    QProcess process;
    QTcpSocket socket;

private:
    void closeResources();
    void onProcessStateChanged(const QProcess::ProcessState &state);
    void onSocketStateChanged(const QTcpSocket::SocketState &state);
    void connectSocket();
    void readError();
    void readOutput();

    std::optional<std::function<void()>> m_connectionHandler;

    enum { None, Terminate, Kill } m_tryClose = None;
};

}

#endif // DAP_SOCKETPROCESSBUS_H
