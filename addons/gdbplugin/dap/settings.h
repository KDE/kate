/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QHash>
#include <QJsonObject>
#include <optional>

class QProcess;

namespace dap
{
namespace settings
{

static const QString REQUEST = QStringLiteral("request");
static const QString RUN_IN_TERMINAL = QStringLiteral("runInTerminal");
static const QString RUN = QStringLiteral("run");
static const QString CONFIGURATIONS = QStringLiteral("configurations");
static const QString COMMAND = QStringLiteral("command");

struct Command {
    QString command;
    QStringList arguments;
    std::optional<QHash<QString, QString>> environment;

    bool isValid() const;
    void start(QProcess &process) const;

    Command() = default;
    explicit Command(const QJsonObject &configuration);
};

struct Connection {
    int port;
    QString host;

    bool isValid() const;
    Connection();
    explicit Connection(const QJsonObject &configuration);
};

struct BusSettings {
    std::optional<Command> command;
    std::optional<Connection> connection;

    bool isValid() const;
    bool hasCommand() const;
    bool hasConnection() const;

    BusSettings() = default;
    explicit BusSettings(const QJsonObject &configuration);
};

struct ProtocolSettings {
    bool linesStartAt1;
    bool columnsStartAt1;
    bool pathFormatURI;
    bool redirectStderr;
    bool redirectStdout;
    bool supportsSourceRequest;
    bool runInTerminal;
    QJsonObject launchRequest;
    QString locale;

    ProtocolSettings();
    explicit ProtocolSettings(const QJsonObject &configuration);
};

struct ClientSettings {
    BusSettings busSettings;
    ProtocolSettings protocolSettings;

    ClientSettings() = default;
    explicit ClientSettings(const QJsonObject &configuration);
    static std::optional<ClientSettings> extractFromAdapter(const QJsonObject &adapterSettings, const QString &configurationKey);
};

std::optional<QJsonObject> expandConfiguration(const QJsonObject &adapterSettings, const QJsonObject &configuration, bool resolvePort = false);
std::optional<QJsonObject> expandConfigurations(const QJsonObject &adapterSettings, bool resolvePort = false);
/**
 * @brief findConfiguration
 *
 * Extract a configuration from an adapter JSON definition.
 *
 * @param adapterSettings
 * @param key
 * @param resolvePort
 * @return
 */
std::optional<QJsonObject> findConfiguration(const QJsonObject &adapterSettings, const QString &key, bool resolvePort = false);
std::optional<QJsonObject> resolveClientPort(const QJsonObject &configuration);

QHash<QString, QJsonValue> findReferences(const QJsonObject &configuration);

}
}
