/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>
#include <QUrl>

#include <optional>

namespace GitForge {
enum class Provider {
    Forgejo,
    GitHub,
    GitLab,
};

struct Remote {
    QString host;
    int port = -1;
    QString repositoryPath;
};

struct HostMapping {
    QString host;
    int port = -1;
    Provider provider = Provider::GitHub;
    QUrl webBaseUrl;
};

struct Repository {
    Provider provider;
    QUrl webBaseUrl;
    QString path;
};

struct LineRange {
    int first = 0;
    int last = 0;
};

std::optional<Remote> parseRemote(const QString &remoteUrl);
std::optional<Repository> resolveRepository(const Remote &remote, const QList<HostMapping> &customMappings);
std::optional<QUrl> blobUrl(const Repository &repository, const QString &ref, const QString &filePath, std::optional<LineRange> lines = std::nullopt);
LineRange selectedLineRange(int startLine, int endLine, int endColumn);

QString providerName(Provider provider);
std::optional<Provider> providerFromName(const QString &name);
std::optional<HostMapping> hostMapping(const QString &host, Provider provider, const QUrl &webBaseUrl);
QList<QPair<Provider, QUrl>> providerApiUrls(const QUrl &webBaseUrl);
bool isProviderApiResponse(Provider provider, int statusCode, const QByteArray &body);
QList<HostMapping> defaultHostMappings();
} // namespace GitForge
