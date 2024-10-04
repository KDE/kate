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

#include "advanced_settings.h"
#include "target_json_keys.h"

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

enum TargetStringOrder { NameIndex = 0, ExecIndex, WorkDirIndex, ArgsIndex, GDBIndex, CustomStartIndex };

inline void upgradeConfigV1_3(QStringList &targetConfStrs)
{
    if (targetConfStrs.count() == 3) {
        // valid old style config, translate it now; note the
        // reordering happening here!
        QStringList temp;
        temp << targetConfStrs[2];
        temp << targetConfStrs[1];
        targetConfStrs.swap(temp);
    }
}

inline void upgradeConfigV3_4(QStringList &targetConfStrs, const QStringList &args)
{
    targetConfStrs.prepend(targetConfStrs[0].right(15));

    const QString targetName(QStringLiteral("%1<%2>"));

    for (int i = 0; i < args.size(); ++i) {
        const QString &argStr = args.at(i);
        if (i > 0) {
            // copy the firsts and change the arguments
            targetConfStrs[0] = targetName.arg(targetConfStrs[0]).arg(i + 1);
            if (targetConfStrs.count() > 3) {
                targetConfStrs[3] = argStr;
            }
        }
    }
}

inline void upgradeConfigV4_5(QStringList targetConfStrs, QJsonObject &conf)
{
    typedef TargetStringOrder I;

    while (targetConfStrs.count() < I::CustomStartIndex) {
        targetConfStrs << QString();
    }

    auto insertField = [&conf, targetConfStrs](const QString &field, I index) {
        const QString value = targetConfStrs[index].trimmed();
        if (!value.isEmpty()) {
            conf[field] = value;
        }
    };

    // read fields
    insertField(TargetKeys::F_TARGET, I::NameIndex);
    insertField(TargetKeys::F_FILE, I::ExecIndex);
    insertField(TargetKeys::F_WORKDIR, I::WorkDirIndex);
    insertField(TargetKeys::F_ARGS, I::ArgsIndex);
    // read advanced settings
    for (int i = 0; i < I::GDBIndex; ++i) {
        targetConfStrs.takeFirst();
    }
    const auto advanced = AdvancedGDBSettings::upgradeConfigV4_5(targetConfStrs);
    if (!advanced.isEmpty()) {
        conf[QStringLiteral("advanced")] = advanced;
    }
}

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
    config.version = group.readEntry(QStringLiteral("version"), CONFIG_VERSION);
    config.targetCount = group.readEntry(QStringLiteral("targetCount"), 1);
    int lastTarget = group.readEntry(QStringLiteral("lastTarget"), 0);
    const QString targetKey(QStringLiteral("target_%1"));

    QStringList args;
    if (config.version < 4) {
        const int argsListsCount = group.readEntry(QStringLiteral("argsCount"), 0);
        const QString argsKey(QStringLiteral("args_%1"));
        const QString targetName(QStringLiteral("%1<%2>"));

        for (int nArg = 0; nArg < argsListsCount; ++nArg) {
            const QString argStr = group.readEntry(argsKey.arg(nArg), QString());
        }
    }

    for (int i = 0; i < config.targetCount; i++) {
        QJsonObject targetConf;

        if (config.version < 5) {
            QStringList targetConfStrs;
            targetConfStrs = group.readEntry(targetKey.arg(i), QStringList());
            if (targetConfStrs.count() == 0) {
                continue;
            }

            if (config.version == 1) {
                upgradeConfigV1_3(targetConfStrs);
            }

            if (config.version < 4) {
                upgradeConfigV3_4(targetConfStrs, args);
            }
            if (config.version < 5) {
                upgradeConfigV4_5(targetConfStrs, targetConf);
            }
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
