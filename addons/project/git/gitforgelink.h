/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "gitforgeurl.h"

#include <optional>

namespace GitForge {
struct Link {
    QUrl url;
    Provider provider;
};

std::optional<Link> linkForFile(const QString &filePath, const QList<HostMapping> &hostMappings, std::optional<LineRange> lines = std::nullopt);
} // namespace GitForge
