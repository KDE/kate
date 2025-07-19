/*
 * This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2025 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
 *
 *  SPDX-License-Identifier: MIT
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QUrl>

#include "../exec_utils.h"

class ExecTest : public QObject
{
    Q_OBJECT

    QJsonDocument parse(const QByteArray &data)
    {
        QJsonParseError error;
        auto json = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug() << "failed parse " << error.errorString();
        }

        return json;
    }

private Q_SLOTS:

    void testMap()
    {
        const char *data = R"|(
        [
          { "localRoot": "/home/me/src", "remoteRoot": "/workdir" },
          [ "/tmp/root", "/" ]
        ]
        )|";

        auto json = parse(QByteArray::fromStdString(data));
        QVERIFY(json.isArray());

        auto smap = Utils::loadMapping(json.array());
        auto &map = *smap;

        QCOMPARE(map.size(), 2);
        qDebug() << map;

        {
            auto p = QUrl::fromLocalFile(QStringLiteral("/home/me/src/foo"));
            auto rp = Utils::mapPath(map, p, true);
            QCOMPARE(rp, QUrl::fromLocalFile(QStringLiteral("/workdir/foo")));
            auto q = Utils::mapPath(map, rp, false);
            QCOMPARE(q, p);
        }

        {
            auto p = QUrl::fromLocalFile(QStringLiteral("/home/me/src"));
            auto rp = Utils::mapPath(map, p, true);
            QCOMPARE(rp, QUrl::fromLocalFile(QStringLiteral("/workdir")));
            auto q = Utils::mapPath(map, rp, false);
            QCOMPARE(q, p);
        }

        {
            auto p = QUrl::fromLocalFile(QStringLiteral("/tmp"));
            auto rp = Utils::mapPath(map, p, false);
            QCOMPARE(rp, QUrl::fromLocalFile(QStringLiteral("/tmp/root/tmp")));
            auto q = Utils::mapPath(map, rp, true);
            QCOMPARE(q, p);
        }

        {
            auto p = QUrl::fromLocalFile(QStringLiteral("/tmp"));
            auto rp = Utils::mapPath(map, p, true);
            QCOMPARE(rp, QUrl());
        }
    }

    void testLoad()
    {
        const char *project_json =
            R"|(
{
  "name": "test",
  "exec": {
      "hostname": "foobar",
      "prefix": "prefix arg",
      "mapRemoteRoot": true,
      "pathMappings": [ ]
  },
  "lspclient": {
    "python": {
      "root": ".",
      "exec": { "hostname": "foobar" }
    }
  }
}
        )|";

        auto projectConfig = parse(QByteArray::fromStdString(project_json));
        QVERIFY(projectConfig.isObject());

        auto pc = projectConfig.object();
        auto sc = pc.value(QStringLiteral("lspclient")).toObject().value(QStringLiteral("python")).toObject();

        auto execConfig = Utils::ExecConfig::load(sc, pc, {});
        QCOMPARE(execConfig.hostname(), QStringLiteral("foobar"));
        auto prefixArray = QJsonArray::fromStringList(QStringList{QStringLiteral("prefix"), QStringLiteral("arg")});
        QCOMPARE(execConfig.prefix().toArray(), prefixArray);
        auto pm = execConfig.init_mapping(nullptr);
        // a fallback should have been arranged
        QCOMPARE(pm->size(), 1);
    }
};

QTEST_MAIN(ExecTest)

#include "exec_tests.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
