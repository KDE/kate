/*
    SPDX-FileCopyrightText: 2025 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "exec_utils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QUrl>

#include <memory>

#include <KSandbox>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include "exec_io_utils.h"
#include "json_utils.h"

#include "kate_exec_debug.h"

namespace Utils
{
// (localRoot = probably file but need not be, remoteRoot = should be file)
using PathMap = std::pair<QUrl, QUrl>;
using PathMapping = QSet<PathMap>;
using PathMappingPtr = std::shared_ptr<PathMapping>;

PathMappingPtr loadMapping(const QJsonValue &json, KTextEditor::View *view)
{
    PathMappingPtr m;

    if (!json.isArray()) {
        return m;
    }

    m.reset(new PathMapping());
    updateMapping(*m, json, view);

    return m;
}

void updateMapping(PathMapping &mapping, const QJsonValue &json, KTextEditor::View *view)
{
    if (!json.isArray())
        return;

    auto editor = KTextEditor::Editor::instance();

    auto make_url = [editor, view](QString v) {
        if (view)
            v = editor->expandText(v, view);
        auto url = QUrl(v);
        // no normalize at this stage
        // so subsequent transformation has clear semantics
        // any normalization can/will happen later if needed
        if (url.isRelative())
            url.setScheme(QStringLiteral("file"));
        return url;
    };

    auto add_entry = [&](QJsonValue local, QJsonValue remote) {
        if (local.isString() && remote.isString()) {
            mapping.insert({make_url(local.toString()), make_url(remote.toString())});
        }
    };

    for (auto e : json.toArray()) {
        // allow various common representations
        if (e.isObject()) {
            auto obj = e.toObject();
            auto local = obj.value(QLatin1String("localRoot"));
            auto remote = obj.value(QLatin1String("remoteRoot"));
            add_entry(local, remote);
        } else if (e.isArray()) {
            auto a = e.toArray();
            if (a.size() == 2) {
                auto local = a[0];
                auto remote = a[1];
                add_entry(local, remote);
            }
        } else if (e.isString()) {
            auto parts = e.toString().split(QLatin1Char(':'));
            if (parts.size() == 2) {
                mapping.insert({make_url(parts[0]), make_url(parts[1])});
            }
        }
    }
}

bool updateMapping(PathMapping &mapping, const QByteArray &data)
{
    auto header = QByteArray("X-Type: Mounts");
    if (data.indexOf(header) < 0)
        return false;

    // find start of what should be JSON array
    auto index = data.indexOf('[');
    if (index < 0)
        return false;

    // parse
    auto payload = data.mid(index);
    QJsonParseError error;
    auto json = QJsonDocument::fromJson(payload, &error);
    qDebug() << "payload" << payload;
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "payload parse failed" << error.errorString();
        qCWarning(LibKateExec) << "payload parse failed" << error.errorString();
        return false;
    }

    updateMapping(mapping, json.array());

    return true;
}

QUrl mapPath(const PathMapping &mapping, const QUrl &p, bool fromLocal)
{
    const auto SEP = QLatin1Char('/');
    const PathMap *entry = nullptr;
    QString suffix;
    QUrl result;

    if (!p.isValid() || !p.path().startsWith(SEP))
        return result;

    for (auto &m : mapping) {
        auto &root = fromLocal ? m.first : m.second;
        // .parentOf does not accept the == case
        if (root.isParentOf(p) || root == p) {
            auto rootPath = root.path(QUrl::FullyEncoded);
            auto suf = p.path(QUrl::FullyEncoded).mid(rootPath.size());
            if (suf.size() && suf[0] == SEP)
                suf.erase(suf.begin());
            if (!entry || suf.size() < suffix.size()) {
                entry = &m;
                suffix = suf;
            }
        }
    }
    if (entry) {
        result = fromLocal ? entry->second : entry->first;
        if (suffix.size()) {
            auto path = result.path(QUrl::FullyEncoded);
            if (path.back() != SEP)
                path.append(SEP);
            path.append(suffix);
            result.setPath(path, QUrl::TolerantMode);
        }
    }

    return result;
}

static void findExec(const QJsonValue &value, const QString &hostname, QJsonObject &current)
{
    auto check = [&hostname](const QJsonObject &ob) {
        return ob.value(QLatin1String("hostname")).toString() == hostname;
    };

    json::find(value, check, current);
}

PathMappingPtr ExecConfig::init_mapping(KTextEditor::View *view)
{
    // load path mapping, with var substitution
    auto pathMapping = Utils::loadMapping(config.value(QLatin1String("pathMappings")), view);
    // check if user has specified map for remote root
    auto rooturl = QUrl::fromLocalFile(QStringLiteral("/"));
    if (pathMapping && Utils::mapPath(*pathMapping, rooturl, false).isEmpty()) {
        auto &epm = Utils::ExecPrefixManager::instance();
        // if not, then add a mapping
        // if enabled, use a kio exec root with specified host
        // otherwise, use same protocol with empty host
        // the latter maps nowhere, but it least it provides both a path
        // and a clear indication not to confuse it with a mere local path
        auto fallback = config.value(QLatin1String("mapRemoteRoot")).toBool();
        auto hn = hostname();
        pathMapping->insert({QUrl(QLatin1String("%1://%2/").arg(epm.scheme(), fallback ? hn : QString())), rooturl});
        if (fallback) {
            // use substituted part of cmdline as prefix
            auto editor = KTextEditor::Editor::instance();
            QStringList sub_prefix;
            for (const auto &e : prefix().toArray()) {
                sub_prefix.push_back(editor->expandText(e.toString(), view));
            }
            epm.update(hn, sub_prefix);
        }
    }
    return pathMapping;
}

ExecConfig ExecConfig::load(const QJsonObject &localConfig, const QJsonObject &projectConfig, QList<QJsonValue> extra)
{
    ExecConfig result;

    // try to collect all exec related info
    constexpr auto EXEC = QLatin1String("exec");
    auto execConfig = localConfig.value(EXEC).toObject();
    QString hostname;
    if (!execConfig.isEmpty()) {
        hostname = execConfig.value(QLatin1String("hostname")).toString();
        // convenience; let's try to find more info for this hostname elsewhere
        if (!hostname.isEmpty()) {
            QJsonObject current;
            // first look into a common project config part
            findExec(projectConfig.value(EXEC), hostname, current);
            // search extra parts
            for (const auto &e : extra)
                findExec(e, hostname, current);
            // merge
            execConfig = json::merge(current, execConfig);
        }
    }
    // normalize string prefix to array
    constexpr auto PREFIX = QLatin1String("prefix");
    if (auto sprefix = execConfig.value(PREFIX).toString(); !sprefix.isEmpty()) {
        execConfig[PREFIX] = QJsonArray::fromStringList(sprefix.split(QLatin1Char(' ')));
    }

    result.config = execConfig;

    return result;
}

void setupFlatpakPathMapping(PathMappingPtr &mapping)
{
    if (KSandbox::isFlatpak()) {
        if (!mapping) {
            mapping.reset(new Utils::PathMapping);
        }
        auto local = QUrl(QStringLiteral("file:///run/host/usr/"));
        auto remote = QUrl(QStringLiteral("file:///usr/"));
        mapping->insert({local, remote});

        local = QUrl(QStringLiteral("file:///home"));
        remote = QUrl(QStringLiteral("file:///home"));
        mapping->insert({local, remote});
    }
}

} // Utils
