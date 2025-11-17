#pragma once

#include <QJsonArray>

QList<QJsonValue> readLaunchJsonConfigs(const QStringList &projectBaseDirs, QStringList &errors);
