/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#include "FlutterSupport.h"

#include "TargetModel.h"
#include <ktexteditor_utils.h>

#include <rapidjson/document.h>

struct FlutterLaunchTarget {
    QString name;
    QString cwd;
    QString runCmd;
};

static QList<FlutterLaunchTarget> readVsCodeLaunchJson(const QString &filePath, const QString &baseDir)
{
    QFile f(filePath);
    if (!f.open(QFile::ReadOnly)) {
        qDebug() << "Failed to open vscode";
        return {};
    }

    const auto data = f.readAll();
    rapidjson::Document doc;
    doc.Parse<rapidjson::kParseCommentsFlag>(data.constData());
    if (doc.HasParseError()) {
        qWarning() << "Failed to parse .vscode/launch.json";
        return {};
    }

    auto it = doc.FindMember("configurations");
    if (it == doc.MemberEnd() || !it->value.IsArray()) {
        return {};
    }

    QList<FlutterLaunchTarget> targets;
    const auto configurations = it->value.GetArray();
    for (const auto &c : configurations) {
        if (!c.IsObject()) {
            continue;
        }
        auto obj = c.GetObject();
        auto it = obj.FindMember("type");
        QByteArrayView type = (it == obj.MemberEnd() || !it->value.IsString()) ? QByteArrayView() : it->value.GetString();
        it = obj.FindMember("request");
        QByteArrayView request = (it == obj.MemberEnd() || !it->value.IsString()) ? QByteArrayView() : it->value.GetString();
        if (type == "dart" && request == "launch") {
            it = obj.FindMember("name");
            QByteArrayView name = (it == obj.MemberEnd() || !it->value.IsString()) ? QByteArrayView() : it->value.GetString();
            it = obj.FindMember("cwd");
            QByteArrayView cwd = (it == obj.MemberEnd() || !it->value.IsString()) ? QByteArrayView() : it->value.GetString();
            it = obj.FindMember("flutterMode");
            QByteArrayView flutterMode = (it == obj.MemberEnd() || !it->value.IsString()) ? QByteArrayView() : it->value.GetString();
            it = obj.FindMember("args");
            it->value.GetArray();

            QStringList args;
            if (it != obj.MemberEnd() && it->value.IsArray()) {
                for (const auto &v : it->value.GetArray()) {
                    if (v.IsString() && v.GetStringLength() != 0) {
                        args.push_back(QString::fromUtf8(v.GetString(), v.GetStringLength()));
                    }
                }
            }

            QString runCmd = QStringLiteral("flutter run");
            if (!args.isEmpty()) {
                runCmd += QLatin1String(" ");
                runCmd += args.join(QLatin1String(" "));
            }
            if (!flutterMode.isEmpty()) {
                runCmd += QLatin1String(" --%1").arg(QString::fromLatin1(flutterMode));
            }

            if (!name.isEmpty()) {
                targets.push_back(FlutterLaunchTarget{
                    .name = QString::fromUtf8(name),
                    .cwd = QDir::cleanPath(baseDir + QLatin1String("/") + QString::fromUtf8(cwd)),
                    .runCmd = runCmd,
                });
            }
        }
    }
    return targets;
}

static QList<FlutterLaunchTarget> discoverLaunchTargets(const QString &baseDir)
{
    if (baseDir.isEmpty()) {
        return {};
    }

    QDir projectBaseDir(baseDir);

    QList<FlutterLaunchTarget> targets;

    const QString vscodeLaunchJson = QStringLiteral(".vscode/launch.json");
    if (!baseDir.isEmpty() && projectBaseDir.exists(vscodeLaunchJson)) {
        targets << readVsCodeLaunchJson(projectBaseDir.absoluteFilePath(vscodeLaunchJson), baseDir);
        qDebug() << "discovered " << targets.size() << "vscode launch.json targets";
    }

    // targets were discovered from launch.json, dont try to create our own because
    // it will most likely lead to duplication
    if (!targets.isEmpty()) {
        return targets;
    }

    if (!baseDir.isEmpty() && projectBaseDir.exists(QStringLiteral("pubspec.yaml"))) {
        const QString name = projectBaseDir.dirName();
        FlutterLaunchTarget t{
            .name = name,
            .cwd = baseDir,
            .runCmd = QStringLiteral("flutter run"),
        };
        targets.push_back(t);

        t.name = QLatin1String("%1 (%2)").arg(name, QLatin1String("profile"));
        t.runCmd = QStringLiteral("flutter run --profile");
        targets.push_back(t);

        qDebug() << "created project flutter targets";
    }
    return targets;
}

void KFlutter::loadFlutterTargets(const QString &projectBaseDir, TargetModel *model, const QModelIndex &targetSetIndex)
{
    const auto flutterLaunchTargets = discoverLaunchTargets(projectBaseDir);
    if (flutterLaunchTargets.isEmpty()) {
        return;
    }

    QHash<QString, QPersistentModelIndex> workingDirToIndex;
    QPersistentModelIndex last = targetSetIndex;
    for (const auto &t : flutterLaunchTargets) {
        if (!workingDirToIndex.contains(t.cwd)) {
            QString folder = Utils::fileNameFromPath(t.cwd);
            if (folder.endsWith(QLatin1String("/"))) {
                folder.chop(1);
            }
            last = model->insertTargetSetAfter(last, folder, t.cwd);
            workingDirToIndex[t.cwd] = last;
        }
    }

    // Add targets to the targetSets created above
    for (const auto &tgt : flutterLaunchTargets) {
        const auto index = workingDirToIndex.value(tgt.cwd);
        if (!index.isValid()) {
            qWarning() << "unexpected invalid index for" << tgt.cwd;
            continue;
        }
        // TODO: its not always flutter
        model->addCommandAfter(index, tgt.name, {}, tgt.runCmd, QStringLiteral("flutter"));
    }
}
