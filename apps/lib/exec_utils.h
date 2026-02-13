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

/**
 * Sets up path mapping for flatpak
 */
KATE_PRIVATE_EXPORT void setupFlatpakPathMapping(PathMappingPtr &mapping);

class KATE_PRIVATE_EXPORT ExecConfig
{
    QJsonObject config;

public:
    // environment var
    static QString ENV_KATE_EXEC_PLUGIN();
    static QString ENV_KATE_EXEC_INSPECT();

    static ExecConfig load(const QJsonObject &localConfig, const QJsonObject &projectConfig, QList<QJsonValue> extra);

    QString hostname() const;

    QJsonValue prefix() const;

    PathMappingPtr init_mapping(KTextEditor::View *view);
};

} // Utils
