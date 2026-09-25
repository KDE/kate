/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

#include <optional>

namespace Deployment {
struct Config {
  QString host;
  int port = 22;
  QString user;
  QString localRoot;
  QString remoteRoot;
  QStringList excludePatterns;
  bool enabled = false;
  bool valid = false;
  QString error;

  static Config fromProjectMap(const QVariantMap &projectMap,
                               const QString &projectBase);

  QUrl remoteBaseUrl() const;
  std::optional<QUrl> remoteUrlForLocalPath(const QString &localPath) const;
  std::optional<QString> localPathForRemoteUrl(const QUrl &url) const;
  bool isExcluded(const QString &localPath) const;
};
} // namespace Deployment
