/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef DAP_BUS_H
#define DAP_BUS_H

#include <QObject>

class QJsonObject;
class QByteArray;

namespace dap
{
namespace settings
{
struct BusSettings;
}

class Bus : public QObject
{
    Q_OBJECT
public:
    enum class State { None, Running, Closed };

    explicit Bus(QObject *parent = nullptr);
    ~Bus() override = default;

    virtual QByteArray read() = 0;
    virtual quint16 write(const QByteArray &data) = 0;

    State state() const;

    virtual bool start(const settings::BusSettings &configuration) = 0;
    virtual void close() = 0;

Q_SIGNALS:
    void readyRead();

    void stateChanged(State state);

    void running();
    void closed();
    void error(const QString &errorMessage);
    void serverOutput(const QString &message);
    void processOutput(const QString &message);

protected:
    void setState(State state);

private:
    State m_state;
};

}

#endif // DAP_BUS_H
