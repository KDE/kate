//
// SPDX-FileCopyrightText: 2010 Ian Wakeling <ian.wakeling@ntlworld.com>
// SPDX-FileCopyrightText: 2012 Kåre Särs <kare.sars@iki.fi>
// SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>
// SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
//
//  SPDX-License-Identifier: LGPL-2.0-only
#pragma once

#include <KConfigGroup>
#include <QJsonDocument>
#include <QJsonObject>

constexpr int CONFIG_VERSION = 5;

namespace DebugPluginSessionConfig
{

struct ConfigData {
    int version;
    int targetCount = 1;
    int lastTarget = 0;
    QList<QJsonObject> targetConfigs;
    bool alwaysFocusOnInput = false;
    bool redirectTerminal = false;
};

enum TargetStringOrder {
    NameIndex = 0,
    ExecIndex,
    WorkDirIndex,
    ArgsIndex,
    GDBIndex,
    CustomStartIndex
};

inline QByteArray serialize(const QJsonObject obj)
{
    const QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Compact);
}

inline QJsonObject unserialize(const QString map)
{
    const auto doc = QJsonDocument::fromJson(map.toLatin1());
    return doc.object();
}

inline ConfigData read(const KConfigGroup &group)
{
    ConfigData config;
    config.version = group.readEntry("version", CONFIG_VERSION);
    config.targetCount = group.readEntry("targetCount", 1);
    int lastTarget = group.readEntry("lastTarget", 0);
    const QString targetKey(QStringLiteral("target_%1"));

    for (int i = 0; i < config.targetCount; i++) {
        QJsonObject targetConf;
        if (config.version < 5) {
            // skip
        } else {
            const QString data = group.readEntry(targetKey.arg(i), QString());
            targetConf = unserialize(data);
        }

        if (!targetConf.isEmpty()) {
            config.targetConfigs.push_back(targetConf);
        }
    }

    if (lastTarget < 0 || lastTarget >= config.targetConfigs.size()) {
        lastTarget = 0;
    }

    config.alwaysFocusOnInput = group.readEntry("alwaysFocusOnInput", false);
    config.redirectTerminal = group.readEntry("redirectTerminal", false);
    return config;
}

inline void write(KConfigGroup &group, const ConfigData &config)
{
    group.writeEntry("version", CONFIG_VERSION);

    QString targetKey(QStringLiteral("target_%1"));

    group.writeEntry("lastTarget", config.lastTarget);
    int targetIdx = 0;
    for (const auto &targetConf : config.targetConfigs) {
        group.writeEntry(targetKey.arg(targetIdx++), serialize(targetConf));
    }
    group.writeEntry("targetCount", targetIdx);
    group.writeEntry("alwaysFocusOnInput", config.alwaysFocusOnInput);
    group.writeEntry("redirectTerminal", config.redirectTerminal);
}

}
