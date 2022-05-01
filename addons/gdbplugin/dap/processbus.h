/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef DAP_PROCESSBUS_H
#define DAP_PROCESSBUS_H

#include <QProcess>

#include "bus.h"

namespace dap
{
class ProcessBus : public Bus
{
    Q_OBJECT
public:
    explicit ProcessBus(QObject *parent = nullptr);
    ~ProcessBus() override;

    QByteArray read() override;
    quint16 write(const QByteArray &data) override;

    QProcess process;

    bool start(const settings::BusSettings &configuration) override;
    void close() override;

private:
    void onStateChanged(QProcess::ProcessState state);
    void onError(QProcess::ProcessError processError);
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void readError();

    enum { None, Terminate, Kill } m_tryClose = None;
};

}

#endif // DAP_PROCESSBUS_H
