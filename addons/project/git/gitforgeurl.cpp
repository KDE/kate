/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitforgeurl.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrlQuery>

#include <algorithm>

namespace {
std::optional<QString> normalizedPath(QString path)
{
    while (path.startsWith(u'/')) {
        path.remove(0, 1);
    }
    while (path.endsWith(u'/')) {
        path.chop(1);
    }
    if (path.endsWith(QLatin1String(".git"))) {
        path.chop(4);
    }

    const QStringList parts = path.split(u'/');
    if (parts.size() < 2) {
        return std::nullopt;
    }
    for (const QString &part : parts) {
        if (part.isEmpty() || part == QLatin1String(".") || part == QLatin1String("..")) {
            return std::nullopt;
        }
    }
    return path;
}

std::optional<QByteArray> encodedPath(const QString &path)
{
    if (path.startsWith(u'/') || path.endsWith(u'/')) {
        return std::nullopt;
    }
    QByteArray result;
    const QStringList parts = path.split(u'/');
    for (const QString &part : parts) {
        if (part.isEmpty() || part == QLatin1String(".") || part == QLatin1String("..")) {
            return std::nullopt;
        }
        if (!result.isEmpty()) {
            result += '/';
        }
        result += QUrl::toPercentEncoding(part);
    }
    return result;
}

QString normalizedHost(const QString &host)
{
    if (host.contains(u':')) {
        const QUrl ipv6Url(QStringLiteral("http://[") + host + QStringLiteral("]/"), QUrl::StrictMode);
        return ipv6Url.isValid() ? ipv6Url.host().toLower() : QString();
    }
    return QString::fromLatin1(QUrl::toAce(host)).toLower();
}

bool validWebBase(const QUrl &url)
{
    const QString scheme = url.scheme().toLower();
    return url.isValid() && (scheme == QLatin1String("http") || scheme == QLatin1String("https")) && !url.host().isEmpty() && url.userInfo().isEmpty()
        && !url.hasQuery() && !url.hasFragment();
}
} // namespace

std::optional<GitForge::Remote> GitForge::parseRemote(const QString &remoteUrl)
{
    QString host;
    QString path;
    int port = -1;

    if (remoteUrl.contains(QLatin1String("://"))) {
        const QUrl url(remoteUrl, QUrl::StrictMode);
        const QString scheme = url.scheme().toLower();
        if (!url.isValid()
            || (scheme != QLatin1String("http") && scheme != QLatin1String("https") && scheme != QLatin1String("ssh") && scheme != QLatin1String("git"))
            || url.host().isEmpty() || url.hasQuery() || url.hasFragment()) {
            return std::nullopt;
        }
        host = url.host();
        port = url.port(-1);
        path = url.path(QUrl::FullyDecoded);
    } else {
        static const QRegularExpression scpPattern(QStringLiteral(R"(^(?:[^@/:]+@)?(\[[^\]]+\]|[^/:]+):(.+)$)"));
        const QRegularExpressionMatch match = scpPattern.match(remoteUrl);
        if (!match.hasMatch()) {
            return std::nullopt;
        }
        host = match.captured(1);
        if (host.startsWith(u'[') && host.endsWith(u']')) {
            host = host.sliced(1, host.size() - 2);
        }
        path = match.captured(2);
    }

    const auto repositoryPath = normalizedPath(path);
    host = normalizedHost(host);
    if (!repositoryPath || host.isEmpty()) {
        return std::nullopt;
    }
    return Remote{ host, port, *repositoryPath };
}

std::optional<GitForge::Repository> GitForge::resolveRepository(const Remote &remote, const QList<HostMapping> &customMappings)
{
    const QString remoteHost = normalizedHost(remote.host);
    const HostMapping *wildcardPortMapping = nullptr;
    for (const HostMapping &mapping : customMappings) {
        if (normalizedHost(mapping.host) != remoteHost || !validWebBase(mapping.webBaseUrl)) {
            continue;
        }
        if (mapping.port == remote.port && mapping.port != -1) {
            wildcardPortMapping = &mapping;
            break;
        }
        if (mapping.port == -1 && !wildcardPortMapping) {
            wildcardPortMapping = &mapping;
        }
    }

    if (wildcardPortMapping) {
        QString repositoryPath = remote.repositoryPath;
        QString basePath = wildcardPortMapping->webBaseUrl.path(QUrl::FullyDecoded);
        while (basePath.startsWith(u'/')) {
            basePath.remove(0, 1);
        }
        while (basePath.endsWith(u'/')) {
            basePath.chop(1);
        }
        if (!basePath.isEmpty() && repositoryPath.startsWith(basePath + u'/')) {
            repositoryPath.remove(0, basePath.size() + 1);
        }
        if (wildcardPortMapping->provider != Provider::GitLab && repositoryPath.count(u'/') != 1) {
            return std::nullopt;
        }
        return Repository{ wildcardPortMapping->provider, wildcardPortMapping->webBaseUrl, repositoryPath };
    }

    return std::nullopt;
}

std::optional<QUrl> GitForge::blobUrl(const Repository &repository, const QString &ref, const QString &filePath, std::optional<LineRange> lines)
{
    const auto repositoryPath = encodedPath(repository.path);
    const auto encodedFilePath = encodedPath(filePath);
    if (!repositoryPath || !encodedFilePath || ref.isEmpty() || !validWebBase(repository.webBaseUrl)) {
        return std::nullopt;
    }
    if (lines && (lines->first < 1 || lines->last < lines->first)) {
        return std::nullopt;
    }

    QByteArray url = repository.webBaseUrl.toEncoded(QUrl::RemoveQuery | QUrl::RemoveFragment);
    while (url.endsWith('/')) {
        url.chop(1);
    }
    url += '/';
    url += *repositoryPath;
    if (repository.provider == Provider::Forgejo) {
        static const QRegularExpression commitPattern(QStringLiteral("^[0-9a-fA-F]{40,64}$"));
        url += commitPattern.match(ref).hasMatch() ? "/src/commit/" : "/src/branch/";
    } else {
        url += repository.provider == Provider::GitHub ? "/blob/" : "/-/blob/";
    }
    url += QUrl::toPercentEncoding(ref, "/");
    url += '/';
    url += *encodedFilePath;

    if (lines) {
        url += "#L" + QByteArray::number(lines->first);
        if (lines->last != lines->first) {
            url += repository.provider == Provider::GitLab ? "-" : "-L";
            url += QByteArray::number(lines->last);
        }
    }

    const QUrl result = QUrl::fromEncoded(url, QUrl::StrictMode);
    return result.isValid() ? std::optional<QUrl>(result) : std::nullopt;
}

GitForge::LineRange GitForge::selectedLineRange(int startLine, int endLine, int endColumn)
{
    if (endLine > startLine && endColumn == 0) {
        --endLine;
    }
    return LineRange{ startLine + 1, endLine + 1 };
}

QString GitForge::providerName(Provider provider)
{
    switch (provider) {
    case Provider::Forgejo:
        return QStringLiteral("Forgejo");
    case Provider::GitHub:
        return QStringLiteral("GitHub");
    case Provider::GitLab:
        return QStringLiteral("GitLab");
    }
    Q_UNREACHABLE();
}

std::optional<GitForge::Provider> GitForge::providerFromName(const QString &name)
{
    if (name.compare(QLatin1String("Forgejo"), Qt::CaseInsensitive) == 0) {
        return Provider::Forgejo;
    }
    if (name.compare(QLatin1String("GitHub"), Qt::CaseInsensitive) == 0) {
        return Provider::GitHub;
    }
    if (name.compare(QLatin1String("GitLab"), Qt::CaseInsensitive) == 0) {
        return Provider::GitLab;
    }
    return std::nullopt;
}

std::optional<GitForge::HostMapping> GitForge::hostMapping(const QString &hostAndPort, Provider provider, const QUrl &webBaseUrl)
{
    QUrl authority(QStringLiteral("ssh://") + hostAndPort + u'/', QUrl::StrictMode);
    if (!authority.isValid() || authority.host().isEmpty() || !authority.userInfo().isEmpty() || authority.path() != QLatin1String("/")) {
        return std::nullopt;
    }
    if (!validWebBase(webBaseUrl)) {
        return std::nullopt;
    }
    const QString host = normalizedHost(authority.host());
    if (host.isEmpty()) {
        return std::nullopt;
    }
    return HostMapping{ host, authority.port(-1), provider, webBaseUrl };
}

QList<GitForge::HostMapping> GitForge::effectiveHostMappings(const QList<HostMapping> &globalMappings, const QVariantMap &projectMap)
{
    QList<HostMapping> mappings;
    const QVariantList entries = projectMap.value(QStringLiteral("git")).toMap().value(QStringLiteral("hostMappings")).toList();
    for (const QVariant &entry : entries) {
        const QVariantMap values = entry.toMap();
        const auto provider = providerFromName(values.value(QStringLiteral("provider")).toString());
        if (provider) {
            if (const auto mapping = hostMapping(values.value(QStringLiteral("host")).toString(),
                    *provider,
                    QUrl(values.value(QStringLiteral("webBaseUrl")).toString(), QUrl::StrictMode))) {
                mappings.push_back(*mapping);
            }
        }
    }

    for (const HostMapping &globalMapping : globalMappings) {
        const auto overridden = std::ranges::find_if(mappings, [&globalMapping](const HostMapping &mapping) {
            return mapping.host == globalMapping.host && mapping.port == globalMapping.port;
        });
        if (overridden == mappings.end()) {
            mappings.push_back(globalMapping);
        }
    }
    return mappings;
}

QList<QPair<GitForge::Provider, QUrl>> GitForge::providerApiUrls(const QUrl &webBaseUrl)
{
    if (!validWebBase(webBaseUrl)) {
        return {};
    }

    QString basePath = webBaseUrl.path();
    while (basePath.endsWith(u'/')) {
        basePath.chop(1);
    }

    QUrl forgejoUrl = webBaseUrl;
    forgejoUrl.setPath(basePath + QStringLiteral("/api/v1/version"));
    QUrl gitLabUrl = webBaseUrl;
    gitLabUrl.setPath(basePath + QStringLiteral("/api/v4/projects"));
    QUrlQuery gitLabQuery;
    gitLabQuery.addQueryItem(QStringLiteral("simple"), QStringLiteral("true"));
    gitLabQuery.addQueryItem(QStringLiteral("per_page"), QStringLiteral("1"));
    gitLabUrl.setQuery(gitLabQuery);
    QUrl gitHubUrl = webBaseUrl;
    gitHubUrl.setPath(basePath + QStringLiteral("/api/v3"));
    return { { Provider::Forgejo, forgejoUrl }, { Provider::GitLab, gitLabUrl }, { Provider::GitHub, gitHubUrl } };
}

bool GitForge::isProviderApiResponse(Provider provider, int statusCode, const QByteArray &body)
{
    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (provider == Provider::Forgejo) {
        return statusCode >= 200 && statusCode < 300 && document.object().value(QStringLiteral("version")).isString();
    }
    if (provider == Provider::GitLab) {
        return (statusCode >= 200 && statusCode < 300 && document.isArray())
            || (statusCode == 401 && document.object().value(QStringLiteral("message")) == QLatin1String("401 Unauthorized"));
    }
    if (statusCode < 200 || statusCode >= 300) {
        return false;
    }
    const QJsonObject object = document.object();
    return object.value(QStringLiteral("current_user_url")).isString() && object.value(QStringLiteral("repository_url")).isString();
}

QList<GitForge::HostMapping> GitForge::defaultHostMappings()
{
    return {
        { QStringLiteral("codeberg.org"), -1, Provider::Forgejo, QUrl(QStringLiteral("https://codeberg.org")) },
        { QStringLiteral("github.com"), -1, Provider::GitHub, QUrl(QStringLiteral("https://github.com")) },
        { QStringLiteral("gitlab.com"), -1, Provider::GitLab, QUrl(QStringLiteral("https://gitlab.com")) },
        { QStringLiteral("invent.kde.org"), -1, Provider::GitLab, QUrl(QStringLiteral("https://invent.kde.org")) },
    };
}
