/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "deploymentconfig.h"

#include <QDir>
#include <QFileInfo>
#include <QMetaType>
#include <QRegularExpression>

namespace {
QString slashPath(QString path) { return QDir::fromNativeSeparators(path); }

bool hasTraversal(const QString &path) {
  const QStringList parts = slashPath(path).split(u'/', Qt::SkipEmptyParts);
  return parts.contains(QLatin1String(".."));
}

bool isWithin(const QString &path, const QString &root) {
  if (path == root) {
    return true;
  }
  const QString prefix = root.endsWith(u'/') ? root : root + u'/';
  return path.startsWith(prefix);
}

std::optional<QString> resolvedLocalPath(const QString &path) {
  QString absolute = slashPath(path);
  if (!QDir::isAbsolutePath(absolute) || hasTraversal(absolute)) {
    return std::nullopt;
  }
  absolute = QDir::cleanPath(absolute);

  QFileInfo info(absolute);
  if (info.exists() || info.isSymLink()) {
    const QString canonical = slashPath(info.canonicalFilePath());
    return canonical.isEmpty() ? std::nullopt
                               : std::optional<QString>(canonical);
  }

  QStringList suffix;
  QString ancestor = absolute;
  while (!QFileInfo::exists(ancestor)) {
    const QFileInfo ancestorInfo(ancestor);
    const QString name = ancestorInfo.fileName();
    const QString parent = slashPath(ancestorInfo.dir().absolutePath());
    if (name.isEmpty() || parent == ancestor) {
      return std::nullopt;
    }
    suffix.prepend(name);
    ancestor = parent;
  }

  const QString canonicalAncestor =
      slashPath(QFileInfo(ancestor).canonicalFilePath());
  if (canonicalAncestor.isEmpty()) {
    return std::nullopt;
  }
  return QDir::cleanPath(canonicalAncestor + u'/' + suffix.join(u'/'));
}

QString relativeTo(const QString &path, const QString &root) {
  if (path == root) {
    return QString();
  }
  const int offset = root.endsWith(u'/') ? root.size() : root.size() + 1;
  return path.sliced(offset);
}

bool validRemotePath(const QString &path) {
  return path.startsWith(u'/') && !hasTraversal(path);
}

std::optional<QString> decodedUrlPath(const QUrl &url) {
  const QString encodedPath = url.path(QUrl::FullyEncoded);
  QStringList decodedParts;
  const QStringList parts = encodedPath.split(u'/', Qt::KeepEmptyParts);
  for (const QString &part : parts) {
    const QString decoded = QUrl::fromPercentEncoding(part.toUtf8());
    if (decoded == QLatin1String(".") || decoded == QLatin1String("..") ||
        decoded.contains(u'/') || decoded.contains(u'\\')) {
      return std::nullopt;
    }
    decodedParts.push_back(decoded);
  }
  const QString path = decodedParts.join(u'/');
  return validRemotePath(path) ? std::optional<QString>(QDir::cleanPath(path))
                               : std::nullopt;
}
} // namespace

Deployment::Config
Deployment::Config::fromProjectMap(const QVariantMap &projectMap,
                                   const QString &projectBase) {
  Config config;
  if (!projectMap.contains(QStringLiteral("deployment"))) {
    config.valid = true;
    return config;
  }

  config.enabled = true;
  const QVariant deploymentValue =
      projectMap.value(QStringLiteral("deployment"));
  if (deploymentValue.metaType().id() != QMetaType::QVariantMap) {
    config.error = QStringLiteral("deployment must be an object");
    return config;
  }
  const QVariantMap values = deploymentValue.toMap();
  config.host = values.value(QStringLiteral("host")).toString().trimmed();
  config.user = values.value(QStringLiteral("user")).toString();
  config.remoteRoot =
      slashPath(values.value(QStringLiteral("remoteRoot")).toString());
  static const QRegularExpression whitespace(QStringLiteral("\\s"));

  auto fail = [&config](const QString &message) {
    config.error = message;
    return config;
  };

  if (config.host.isEmpty()) {
    return fail(QStringLiteral("deployment host must not be empty"));
  }
  if (config.host.contains(u'/') || config.host.contains(u'@') ||
      config.host.contains(whitespace)) {
    return fail(QStringLiteral("deployment host is invalid"));
  }
  if (config.host.startsWith(u'[') && config.host.endsWith(u']')) {
    if (!config.host.contains(u':')) {
      return fail(QStringLiteral("deployment host is invalid"));
    }
    config.host = config.host.sliced(1, config.host.size() - 2);
  }

  if (values.contains(QStringLiteral("port"))) {
    bool ok = false;
    const int parsedPort = values.value(QStringLiteral("port")).toInt(&ok);
    if (!ok || parsedPort < 1 || parsedPort > 65535) {
      return fail(
          QStringLiteral("deployment port must be between 1 and 65535"));
    }
    config.port = parsedPort;
  }

  if (projectBase.trimmed().isEmpty()) {
    return fail(QStringLiteral("project base must not be empty"));
  }
  const QFileInfo projectInfo(projectBase);
  const QString canonicalProjectBase =
      slashPath(projectInfo.canonicalFilePath());
  if (!projectInfo.isDir() || canonicalProjectBase.isEmpty()) {
    return fail(QStringLiteral("project base must be an existing directory"));
  }

  QString configuredLocalRoot =
      values.value(QStringLiteral("localRoot")).toString();
  if (configuredLocalRoot.isEmpty()) {
    configuredLocalRoot = canonicalProjectBase;
  } else if (hasTraversal(configuredLocalRoot)) {
    return fail(
        QStringLiteral("deployment localRoot must not contain traversal"));
  } else if (QDir::isRelativePath(configuredLocalRoot)) {
    configuredLocalRoot = canonicalProjectBase + u'/' + configuredLocalRoot;
  }
  const QFileInfo localInfo(configuredLocalRoot);
  config.localRoot = slashPath(localInfo.canonicalFilePath());
  if (!localInfo.isDir() || config.localRoot.isEmpty() ||
      !isWithin(config.localRoot, canonicalProjectBase)) {
    return fail(QStringLiteral("deployment localRoot must be an existing "
                               "directory inside the project base"));
  }

  if (!validRemotePath(config.remoteRoot)) {
    return fail(QStringLiteral(
        "deployment remoteRoot must be an absolute path without traversal"));
  }
  config.remoteRoot = QDir::cleanPath(config.remoteRoot);

  const QVariant excludeValue = values.value(QStringLiteral("exclude"));
  if (excludeValue.isValid()) {
    if (excludeValue.metaType().id() == QMetaType::QStringList) {
      config.excludePatterns = excludeValue.toStringList();
    } else if (excludeValue.metaType().id() == QMetaType::QVariantList) {
      const auto patterns = excludeValue.toList();
      for (const QVariant &pattern : patterns) {
        if (pattern.metaType().id() != QMetaType::QString) {
          return fail(
              QStringLiteral("deployment exclude entries must be strings"));
        }
        config.excludePatterns.push_back(pattern.toString());
      }
    } else {
      return fail(
          QStringLiteral("deployment exclude must be a list of strings"));
    }
  }

  const QUrl baseUrl = config.remoteBaseUrl();
  if (!baseUrl.isValid() || baseUrl.host().isEmpty()) {
    return fail(QStringLiteral("deployment host is invalid"));
  }
  config.host = baseUrl.host();

  config.valid = true;
  return config;
}

QUrl Deployment::Config::remoteBaseUrl() const {
  QUrl url;
  url.setScheme(QStringLiteral("sftp"));
  url.setHost(host);
  url.setPort(port);
  url.setUserName(user);
  url.setPath(remoteRoot);
  return url;
}

std::optional<QUrl>
Deployment::Config::remoteUrlForLocalPath(const QString &localPath) const {
  if (!enabled || !valid) {
    return std::nullopt;
  }
  const auto resolved = resolvedLocalPath(localPath);
  if (!resolved || !isWithin(*resolved, localRoot)) {
    return std::nullopt;
  }

  QUrl url = remoteBaseUrl();
  const QString relative = relativeTo(*resolved, localRoot);
  QString remotePath = remoteRoot;
  if (!relative.isEmpty()) {
    if (remoteRoot != QLatin1String("/")) {
      remotePath += u'/';
    }
    remotePath += relative;
  }
  url.setPath(remotePath);
  return url.isValid() ? std::optional<QUrl>(url) : std::nullopt;
}

std::optional<QString>
Deployment::Config::localPathForRemoteUrl(const QUrl &url) const {
  if (!enabled || !valid || !url.isValid() ||
      url.scheme().compare(QLatin1String("sftp"), Qt::CaseInsensitive) != 0 ||
      url.host().compare(host, Qt::CaseInsensitive) != 0 ||
      url.port(22) != port || url.userName() != user ||
      !url.password().isEmpty() || url.hasQuery() || url.hasFragment()) {
    return std::nullopt;
  }
  const auto remotePath = decodedUrlPath(url);
  if (!remotePath || !isWithin(*remotePath, remoteRoot)) {
    return std::nullopt;
  }

  const QString relative = relativeTo(*remotePath, remoteRoot);
  const QString candidate =
      relative.isEmpty() ? localRoot : localRoot + u'/' + relative;
  const auto resolved = resolvedLocalPath(candidate);
  if (!resolved || !isWithin(*resolved, localRoot)) {
    return std::nullopt;
  }
  return *resolved;
}

bool Deployment::Config::isExcluded(const QString &localPath) const {
  if (!enabled || !valid) {
    return false;
  }
  const auto resolved = resolvedLocalPath(localPath);
  if (!resolved || !isWithin(*resolved, localRoot)) {
    return false;
  }
  const QString relative = relativeTo(*resolved, localRoot);
  if (relative.isEmpty()) {
    return false;
  }
  const QStringList components = relative.split(u'/', Qt::SkipEmptyParts);
  for (QString pattern : excludePatterns) {
    pattern = slashPath(pattern.trimmed());
    if (pattern.isEmpty() || hasTraversal(pattern) ||
        pattern.startsWith(u'/')) {
      continue;
    }
    const QRegularExpression expression(
        QRegularExpression::wildcardToRegularExpression(pattern));
    if (!expression.isValid()) {
      continue;
    }
    if (!pattern.contains(u'/')) {
      for (const QString &component : components) {
        if (expression.match(component).hasMatch()) {
          return true;
        }
      }
    } else {
      QString prefix;
      for (const QString &component : components) {
        prefix += prefix.isEmpty() ? component : u'/' + component;
        if (expression.match(prefix).hasMatch()) {
          return true;
        }
      }
    }
  }
  return false;
}
