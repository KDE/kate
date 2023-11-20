/*
    SPDX-FileCopyrightText: 2023 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "test_gdbbackend.h"

#include "../gdbbackend.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QTest>
#include <qtestcase.h>

QTEST_MAIN(TestGdbBackend)

void TestGdbBackend::parseBreakpoint()
{
    QFETCH(QJsonObject, msg);
    QFETCH(int, number);
    QFETCH(int, line);
    QFETCH(QString, filename);

    const auto bpoint = BreakPoint::parse(msg);

    QCOMPARE(bpoint.number, number);
    QCOMPARE(bpoint.line, line);
    QCOMPARE(bpoint.file.toLocalFile(), filename);
}

void TestGdbBackend::parseBreakpoint_data()
{
    QTest::addColumn<QJsonObject>("msg");
    QTest::addColumn<int>("number");
    QTest::addColumn<int>("line");
    QTest::addColumn<QString>("filename");

    // GDB: unresolved file (unknown type or unloaded library)
    QTest::newRow("gdb_pending") << QJsonObject{{QStringLiteral("number"), QStringLiteral("1")},
                                                {QStringLiteral("pending"), QStringLiteral("/tests/test.py:44")},
                                                {QStringLiteral("original-location"), QStringLiteral("/tests/test.py:44")}}
                                 << 1 << 44 << QStringLiteral("/tests/test.py");

    // GDB: available file of a known type
    QTest::newRow("gdb_resolved") << QJsonObject{{QStringLiteral("number"), QStringLiteral("2")},
                                                 {QStringLiteral("file"), QStringLiteral("main.cpp")},
                                                 {QStringLiteral("fullname"), QStringLiteral("/tests/main.cpp")},
                                                 {QStringLiteral("line"), QStringLiteral("26")},
                                                 {QStringLiteral("original-location"), QStringLiteral("/tests/main.cpp:25")}}
                                  << 2 << 26 << QStringLiteral("/tests/main.cpp");

    // GDB: resolution of a previous pending breakpoint
    QTest::newRow("gdb_pending_resolved") << QJsonObject{{QStringLiteral("number"), QStringLiteral("1")},
                                                         {QStringLiteral("file"), QStringLiteral("main.cpp")},
                                                         {QStringLiteral("fullname"), QStringLiteral("/tests/main.cpp")},
                                                         {QStringLiteral("line"), QStringLiteral("130")},
                                                         {QStringLiteral("original-location"), QStringLiteral("/tests/main.cpp:129")}}
                                          << 1 << 130 << QStringLiteral("/tests/main.cpp");

    // GDB: multiple breakpoints (eg. end of program)
    QTest::newRow("gdb_multiple") << QJsonObject{{QStringLiteral("number"), QStringLiteral("2")},
                                                 {QStringLiteral("original-location"), QStringLiteral("/tests/main.cpp:36")},
                                                 {QStringLiteral("locations"),
                                                  QJsonArray{
                                                      QJsonObject{
                                                          {QStringLiteral("number"), QStringLiteral("2.1")},
                                                          {QStringLiteral("file"), QStringLiteral("main.cpp")},
                                                          {QStringLiteral("fullname"), QStringLiteral("/tests/main.cpp")},
                                                          {QStringLiteral("line"), QStringLiteral("36")},
                                                      },
                                                      QJsonObject{
                                                          {QStringLiteral("number"), QStringLiteral("2.2")},
                                                          {QStringLiteral("file"), QStringLiteral("main.cpp")},
                                                          {QStringLiteral("fullname"), QStringLiteral("/tests/main.cpp")},
                                                          {QStringLiteral("line"), QStringLiteral("36")},
                                                      },
                                                  }}}
                                  << 2 << 36 << QStringLiteral("/tests/main.cpp");

    // LLDB-MI: unresolved file (unknown type or unloaded library)
    QTest::newRow("lldb_pending") << QJsonObject{{QStringLiteral("number"), QStringLiteral("1")},
                                                 {QStringLiteral("pending"), QJsonArray{QStringLiteral("/tests/test.py:42")}},
                                                 {QStringLiteral("file"), QStringLiteral("??")},
                                                 {QStringLiteral("fullname"), QStringLiteral("\?\?/??")},
                                                 {QStringLiteral("original-location"), QStringLiteral("/tests/test.py:42")}}
                                  << 1 << 42 << QStringLiteral("/tests/test.py");

    // LLDB-MI: available file of a known type
    QTest::newRow("lldb_resolved") << QJsonObject{{QStringLiteral("number"), QStringLiteral("1")},
                                                  {QStringLiteral("pending"), QJsonArray{QStringLiteral("/tests/main.cpp:25")}},
                                                  {QStringLiteral("line"), QStringLiteral("26")},
                                                  {QStringLiteral("file"), QStringLiteral("test.py")},
                                                  {QStringLiteral("fullname"), QStringLiteral("/tests/main.cpp")},
                                                  {QStringLiteral("original-location"), QStringLiteral("/tests/main.cpp:25")}}
                                   << 1 << 26 << QStringLiteral("/tests/main.cpp");

    // LLDB-MI: resolution of a previous pending breakpoint
    QTest::newRow("lldb_resolved") << QJsonObject{{QStringLiteral("number"), QStringLiteral("1")},
                                                  {QStringLiteral("pending"), QJsonArray{QStringLiteral("/tests/solib.cpp:129")}},
                                                  {QStringLiteral("line"), QStringLiteral("130")},
                                                  {QStringLiteral("file"), QStringLiteral("solib.cpp")},
                                                  {QStringLiteral("fullname"), QStringLiteral("/tests/solib.cpp")},
                                                  {QStringLiteral("original-location"), QStringLiteral("/tests/solib.cpp:129")}}
                                   << 1 << 130 << QStringLiteral("/tests/solib.cpp");
}

#include "moc_test_gdbbackend.cpp"
