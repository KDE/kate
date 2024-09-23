/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#pragma once

#include <QDir>
#include <QString>

class TargetModel;

namespace KFlutter
{

/**
 * Load flutter targets if we find any "pubspec.yaml" file in the project root
 * or if we find that .vscode/launch.json file contains any flutter targets
 */
void loadFlutterTargets(const QString &projectBaseDir, TargetModel *model, const QModelIndex &targetSetIndex);

}
