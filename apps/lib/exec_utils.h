/*
    SPDX-FileCopyrightText: 2025 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QUrl>

#include <memory>

#include "kateprivate_export.h"

namespace KTextEditor
{
class View;
}

namespace Utils
{
// (localRoot = probably file but need not be, remoteRoot = should be file)
using PathMap = std::pair<QUrl, QUrl>;
using PathMapping = QSet<PathMap>;
using PathMappingPtr = std::shared_ptr<PathMapping>;

KATE_PRIVATE_EXPORT PathMappingPtr loadMapping(const QJsonValue &json, KTextEditor::View *view = nullptr);

KATE_PRIVATE_EXPORT void updateMapping(PathMapping &mapping, const QJsonValue &json, KTextEditor::View *view = nullptr);

KATE_PRIVATE_EXPORT bool updateMapping(PathMapping &mapping, const QByteArray &extraData);

// tries to map, returns empty QUrl if not possible
KATE_PRIVATE_EXPORT QUrl mapPath(const PathMapping &mapping, const QUrl &p, bool fromLocal);

class KATE_PRIVATE_EXPORT ExecConfig
{
    QJsonObject config;

public:
    static inline QString M_HOSTNAME = QStringLiteral("hostname");
    static inline QString M_PREFIX = QStringLiteral("prefix");
    // environment var
    static inline QString ENV_KATE_EXEC_PLUGIN = QStringLiteral("KATE_EXEC_PLUGIN");
    static inline QString ENV_KATE_EXEC_INSPECT = QStringLiteral("KATE_EXEC_INSPECT");

    static ExecConfig load(const QJsonObject &localConfig, const QJsonObject &projectConfig, QList<QJsonValue> extra);

    QString hostname()
    {
        return config.value(M_HOSTNAME).toString();
    }

    QJsonValue prefix()
    {
        return config.value(M_PREFIX);
    }

    PathMappingPtr init_mapping(KTextEditor::View *view);
};

} // Utils
