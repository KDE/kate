#include "launch_json_reader.h"
#include "target_json_keys.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <KLocalizedString>

#include <rapidjson/cursorstreamwrapper.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

static constexpr QLatin1String KEY_PROGRAM("program");

static QByteArray rapidJsonStringify(const rapidjson::Value &v)
{
    rapidjson::StringBuffer buf;
    rapidjson::Writer w(buf);
    v.Accept(w);
    return QByteArray(buf.GetString(), buf.GetSize());
}

static QJsonArray readVsCodeLaunchJson(const QString &filePath, QString &errorString)
{
    QFile f(filePath);
    if (!f.open(QFile::ReadOnly)) {
        return {};
    }

    const auto data = f.readAll();
    // need to use rapidjson because QJson doesn't support comments'
    rapidjson::Document doc;
    constexpr auto flags = rapidjson::kParseCommentsFlag;
    doc.Parse<flags>(data.constData());
    if (doc.HasParseError()) {
        errorString = i18nc("%1 refers to filepath", "Failed to parse .vscode/launch.json at %1. ", filePath);
        errorString.append(QString::fromUtf8(GetParseError_En(doc.GetParseError())));

        // Get line column info
        rapidjson::StringStream ss(data.constData());
        rapidjson::CursorStreamWrapper cursor(ss);
        rapidjson::Document doc;
        doc.ParseStream<flags>(cursor);
        if (doc.HasParseError()) {
            int l = cursor.GetLine();
            int c = cursor.GetColumn();
            errorString.append(i18nc("at line no:column no", " at %1:%2", l, c));
        }
        return {};
    }

    auto it = doc.FindMember("configurations");
    if (it == doc.MemberEnd() || !it->value.IsArray()) {
        return {};
    }

    const auto str = rapidJsonStringify(it->value);
    QJsonDocument qj = QJsonDocument::fromJson(str);
    return qj.array();
}

static void resolveVSCodeVars(QString &in, const QDir &baseDir)
{
    if (in.contains(QLatin1String("${workspaceFolder}"))) {
        in.replace(QLatin1String("${workspaceFolder}"), baseDir.absolutePath());
    } else if (in.contains(QLatin1String("${workspaceFolderBasename}"))) {
        in.replace(QLatin1String("${workspaceFolderBasename}"), baseDir.dirName());
    }
}

static QJsonObject toKateTarget(const QJsonObject &in, const QDir &baseDir)
{
    using namespace TargetKeys;
    QJsonObject ret = in;
    QString cwd = in.value(QLatin1String("cwd")).toString();
    if (cwd.isEmpty()) {
        cwd = baseDir.absolutePath();
    }
    resolveVSCodeVars(cwd, baseDir);
    QDir cwdDir(cwd);
    if (!cwdDir.isAbsolute()) {
        cwdDir = baseDir.absoluteFilePath(cwd);
    }
    ret[F_WORKDIR] = cwdDir.absolutePath();

    auto it = in.find(F_ARGS);
    if (it != in.end()) {
        auto argsArray = it->toArray();
        QString args;
        for (const auto &a : argsArray) {
            QString arg = a.toString();
            if (!arg.isEmpty()) {
                args += arg;
                args += QLatin1String(" ");
            }
        }
        if (args.endsWith(QLatin1String(" "))) {
            args.chop(1);
        }
        ret[F_ARGS] = args;
    }

    ret[F_DEBUGGER] = in.value(QLatin1String("type"));

    QString program = in.value(KEY_PROGRAM).toString();
    if (!program.isEmpty()) {
        resolveVSCodeVars(cwd, baseDir);
        ret[F_FILE] = program;
    }
    ret[F_IS_LAUNCH_JSON] = true;
    ret[F_LAUNCH_JSON_PROJECT] = baseDir.absolutePath();
    return ret;
}

static QJsonObject processDartFlutterTarget(const QJsonObject &in)
{
    using namespace TargetKeys;
    QJsonObject ret = in;
    const QDir cwdDir(in.value(F_WORKDIR).toString());
    // TODO add better detection, this only detects standard config
    const bool hasLibMain = cwdDir.exists(QStringLiteral("lib/main.dart"));
    bool isFlutter = hasLibMain;
    if (isFlutter) {
        ret[F_DEBUGGER] = QStringLiteral("flutter");
    } else {
        ret[F_DEBUGGER] = QStringLiteral("dart");
    }
    const auto program = in.value(KEY_PROGRAM);

    if (program.isNull() || program.isUndefined()) {
        if (hasLibMain) {
            ret[F_FILE] = cwdDir.filePath(QStringLiteral("lib/main.dart"));
        } else {
            ret[F_FILE] = cwdDir.filePath(QStringLiteral("bin/%1").arg(cwdDir.dirName()));
        }
    }
    return ret;
}

static void postProcessTargets(QJsonArray &configs, const QDir &projectBaseDir)
{
    for (int i = 0; i < configs.size(); ++i) {
        auto obj = configs[i].toObject();
        const QString type = obj.value(QStringLiteral("type")).toString();
        obj = toKateTarget(obj, projectBaseDir);
        if (type == QStringLiteral("dart")) {
            obj = processDartFlutterTarget(obj);
        }
        configs[i] = obj;
    }
}

QList<QJsonValue> readLaunchJsonConfigs(const QStringList &baseDirs, QStringList &errors)
{
    if (baseDirs.isEmpty()) {
        return {};
    }

    QList<QJsonValue> configs;
    for (const QString &baseDir : baseDirs) {
        QDir projectBaseDir(baseDir);
        const QString vscodeLaunchJson = QStringLiteral(".vscode/launch.json");
        QString error;
        if (!baseDir.isEmpty() && projectBaseDir.exists(vscodeLaunchJson)) {
            auto projectConfig = readVsCodeLaunchJson(projectBaseDir.absoluteFilePath(vscodeLaunchJson), error);
            if (!error.isEmpty()) {
                errors.push_back(error);
            }
            postProcessTargets(projectConfig, projectBaseDir);
            configs.reserve(configs.size() + projectConfig.size());
            for (const auto &value : projectConfig) {
                configs.insert(configs.end(), value);
            }
        }
    }
    return configs;
}
