/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "dapclient_debug.h"

#include <QJsonArray>
#include <QLocale>
#include <QProcess>
#include <QString>
#include <random>

#include "settings.h"

namespace dap
{
#include <json_utils.h>
}

namespace dap
{
namespace settings
{
static const QString RUN = QStringLiteral("run");
static const QString CONFIGURATIONS = QStringLiteral("configurations");
static const QString REQUEST = QStringLiteral("request");
static const QString COMMAND = QStringLiteral("command");
static const QString COMMAND_ARGS = QStringLiteral("commandArgs");
static const QString PORT = QStringLiteral("port");
static const QString HOST = QStringLiteral("host");
static const QString REDIRECT_STDERR = QStringLiteral("redirectStderr");
static const QString REDIRECT_STDOUT = QStringLiteral("redirectStdout");

std::random_device rd;
std::default_random_engine rng(rd());
std::uniform_int_distribution<> randomPort(40000, 65535);

bool checkSection(const QJsonObject &data, const QString &key)
{
    if (!data.contains(key)) {
        qCWarning(DAPCLIENT) << "required section '" << key << "' not found";
        return false;
    }
    if (!data[key].isObject()) {
        qCWarning(DAPCLIENT) << "section '" << key << "' is not an object";
        return false;
    }
    return true;
}

bool checkArray(const QJsonObject &data, const QString &key)
{
    return data.contains(key) && data[key].isArray();
}

std::optional<QJsonObject> expandConfiguration(const QJsonObject &adapterSettings, const QJsonObject &configuration, bool resolvePort)
{
    auto out = json::merge(adapterSettings[RUN].toObject(), configuration);

    // check request
    if (!checkSection(out, REQUEST)) {
        return std::nullopt;
    }

    const bool withProcess = checkArray(out, COMMAND);
    const bool withSocket = out.contains(PORT) && out[PORT].isDouble();

    if (!withProcess && !withSocket) {
        qCWarning(DAPCLIENT) << "'run' requires 'command: string[]' or 'port: number'";
        return std::nullopt;
    }

    // check command
    if (withProcess && checkArray(out, COMMAND_ARGS)) {
        auto command = out[COMMAND].toArray();
        for (const auto &item : out[COMMAND_ARGS].toArray()) {
            command << item;
        }
        out[COMMAND] = command;
        out.remove(COMMAND_ARGS);
    }

    // check port
    if (withSocket) {
        int port = out[PORT].toInt(-1);
        if ((port == 0) && resolvePort) {
            port = randomPort(rng);
            out[PORT] = port;
        }
        if (port < 0) {
            qCWarning(DAPCLIENT) << "'port' must be a positive integer or 0";
            return std::nullopt;
        }
    }

    return out;
}

std::optional<QJsonObject> expandConfigurations(const QJsonObject &adapterSettings, bool resolvePort)
{
    if (!checkSection(adapterSettings, RUN)) {
        return std::nullopt;
    }
    if (!checkSection(adapterSettings, CONFIGURATIONS)) {
        return std::nullopt;
    }

    const auto &confs = adapterSettings[CONFIGURATIONS].toObject();

    QJsonObject out;

    for (auto it = confs.constBegin(); it != confs.constEnd(); ++it) {
        const auto profile = expandConfiguration(adapterSettings, it.value().toObject(), resolvePort);
        if (profile) {
            out[it.key()] = *profile;
        }
    }

    return out;
}

std::optional<QJsonObject> findConfiguration(const QJsonObject &adapterSettings, const QString &configurationKey, bool resolvePort)
{
    if (!checkSection(adapterSettings, RUN)) {
        return std::nullopt;
    }
    if (!checkSection(adapterSettings, CONFIGURATIONS)) {
        return std::nullopt;
    }

    const auto &confs = adapterSettings[CONFIGURATIONS].toObject();

    if (!checkSection(confs, configurationKey)) {
        return std::nullopt;
    }

    return expandConfiguration(adapterSettings, confs[configurationKey].toObject(), resolvePort);
}

QHash<QString, QJsonValue> findReferences(const QJsonObject &configuration)
{
    QHash<QString, QJsonValue> variables;

    if (configuration.contains(PORT)) {
        variables[QStringLiteral("#run.port")] = QString::number(configuration[PORT].toInt(-1));
    }
    if (configuration.contains(HOST)) {
        variables[QStringLiteral("#run.host")] = configuration[HOST].toString();
    }

    return variables;
}

std::optional<QJsonObject> resolveClientPort(const QJsonObject &configuration)
{
    int port = configuration[PORT].toInt(-1);

    if (port == 0) {
        QJsonObject out(configuration);
        out[PORT] = randomPort(rng);
        return out;
    }
    return std::nullopt;
}

std::optional<QStringList> toStringList(const QJsonObject &configuration, const QString &key)
{
    const auto &field = configuration[key];
    if (field.isNull() || field.isUndefined() || !field.isArray()) {
        return std::nullopt;
    }
    const auto &array = field.toArray();

    QStringList parts;

    for (const auto &value : array) {
        if (!value.isString()) {
            return std::nullopt;
        }
        parts << value.toString();
    }

    return parts;
}

std::optional<QHash<QString, QString>> toStringHash(const QJsonObject &configuration, const QString &key)
{
    const auto &field = configuration[key];
    if (field.isNull() || field.isUndefined() || !field.isObject()) {
        return std::nullopt;
    }
    const auto &object = field.toObject();
    if (object.isEmpty()) {
        return QHash<QString, QString>();
    }

    QHash<QString, QString> map;

    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!it.value().isString()) {
            return std::nullopt;
        }
        map[it.key()] = it.value().toString();
    }

    return map;
}

/*
 * Command
 */
bool Command::isValid() const
{
    return !command.isEmpty();
}

void Command::start(QProcess &process) const
{
    if (environment) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (auto it = environment->begin(); it != environment->end(); ++it) {
            env.insert(it.key(), it.value());
        }
        process.setProcessEnvironment(env);
    }
    //    qDebug() << process.environment();
    //    qDebug() << process.program();
    process.start(command, arguments);
}

Command::Command(const QJsonObject &configuration)
    : environment(toStringHash(configuration, QStringLiteral("environment")))
{
    auto cmdParts = toStringList(configuration, COMMAND);

    if (cmdParts && !cmdParts->isEmpty()) {
        command = cmdParts->at(0);
        cmdParts->removeFirst();
        if (!cmdParts->isEmpty()) {
            arguments = *cmdParts;
        }
    }
}

/*
 * Connection
 */
bool Connection::isValid() const
{
    return (port > 0) && !host.isEmpty();
}

Connection::Connection()
    : port(-1)
    , host(QStringLiteral("127.0.0.1"))
{
}

Connection::Connection(const QJsonObject &configuration)
    : port(configuration[PORT].toInt(-1))
    , host(QStringLiteral("127.0.0.1"))
{
}

/*
 * BusSettings
 */
bool BusSettings::isValid() const
{
    return hasCommand() || hasConnection();
}

bool BusSettings::hasCommand() const
{
    return command && command->isValid();
}

bool BusSettings::hasConnection() const
{
    return connection && connection->isValid();
}

BusSettings::BusSettings(const QJsonObject &configuration)
    : command(Command(configuration))
    , connection(Connection(configuration))
{
}

/*
 * ProtocolSettings
 */
ProtocolSettings::ProtocolSettings()
    : linesStartAt1(true)
    , columnsStartAt1(true)
    , pathFormatURI(false)
    , redirectStderr(false)
    , redirectStdout(false)
    , supportsSourceRequest(true)
    , locale(QLocale::system().name())
{
}

ProtocolSettings::ProtocolSettings(const QJsonObject &configuration)
    : linesStartAt1(true)
    , columnsStartAt1(true)
    , pathFormatURI(false)
    , redirectStderr(configuration[REDIRECT_STDERR].toBool(false))
    , redirectStdout(configuration[REDIRECT_STDOUT].toBool(false))
    , supportsSourceRequest(configuration[QStringLiteral("supportsSourceRequest")].toBool(true))
    , launchRequest(configuration[REQUEST].toObject())
    , locale(QLocale::system().name())
{
}

/*
 * ClientSettings
 */
ClientSettings::ClientSettings(const QJsonObject &configuration)
    : busSettings(configuration)
    , protocolSettings(configuration)
{
}

std::optional<ClientSettings> ClientSettings::extractFromAdapter(const QJsonObject &adapterSettings, const QString &configurationKey)
{
    const auto configuration = findConfiguration(adapterSettings, configurationKey);
    if (!configuration)
        return std::nullopt;

    return ClientSettings(*configuration);
}

} // settings
} // dap
