/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "test_gdbmi.h"

#include "../gdbmi/parser.h"
#include "../gdbmi/tokens.h"

#include <QSignalSpy>
#include <QString>
#include <QTest>

QTEST_MAIN(TestGdbmi)

void TestGdbmi::tryToken()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(int, value);

    const auto tok = gdbmi::tryToken(msg.toUtf8(), start);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QVERIFY(tok.value.has_value());
        QCOMPARE(tok.value.value(), value);
        QCOMPARE(tok.position, position);
    }
}

void TestGdbmi::tryToken_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("value");

    QTest::newRow("1234") << "1234" << 0 << false << false << 4 << 1234;
    QTest::newRow("1234a") << "1234a" << 0 << false << false << 4 << 1234;
    QTest::newRow("1234\\n") << "1234\n" << 0 << false << false << 4 << 1234;
    QTest::newRow("__1234") << "__1234" << 2 << false << false << 6 << 1234;
    QTest::newRow("asdf") << "asdf" << 0 << true << false << 0 << -1;
}

void TestGdbmi::advanceBlanks()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(int, position);

    const int pos = gdbmi::advanceBlanks(msg.toUtf8(), start);
    QCOMPARE(pos, position);
}

void TestGdbmi::advanceBlanks_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("position");

    QTest::newRow(" \\t  asdf") << " \t  asdf" << 0 << 4;
    QTest::newRow(" \\t  \\nasdf") << " \t  \nasdf" << 0 << 4;
    QTest::newRow(" \\t  \\nasdf[2]") << " \t  \nasdf" << 2 << 4;
    QTest::newRow("asdf") << "asdf" << 2 << 2;
}

void TestGdbmi::tryString()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(QString, value);

    const auto tok = gdbmi::tryString(msg.toUtf8(), start);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QVERIFY(tok.value.has_value());
        QCOMPARE(tok.value.value(), value);
        QCOMPARE(tok.position, position);
    }
}

void TestGdbmi::tryString_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<QString>("value");

    QTest::newRow("\"12\\n34\"") << R"-("12\n34")-" << 0 << false << false << 8 << "12\n34";
    QTest::newRow("\"\"") << "\"\"" << 0 << false << false << 2 << "";
    // base
    QTest::newRow("\"1234\"") << "\"1234\"" << 0 << false << false << 6 << "1234";
    // escaped quote
    QTest::newRow("\"12\\\"34\"") << R"-("12\\\"34")-" << 0 << false << false << 10 << R"-(12\"34)-";
    // incomplete (start)
    QTest::newRow("\"1234\"") << "\"1234\"" << 1 << true << true << 0 << "";
    // incomplete (end)
    QTest::newRow("\"1234") << "\"1234" << 0 << true << true << 0 << "";
}

void TestGdbmi::tryClassName()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(QString, value);

    const auto tok = gdbmi::tryClassName(msg.toUtf8(), start);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QVERIFY(tok.value.has_value());
        QCOMPARE(tok.value.value(), value);
        QCOMPARE(tok.position, position);
    }
}

void TestGdbmi::tryClassName_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<QString>("value");

    QTest::newRow("^exit") << "^exit" << 1 << false << false << 5 << "exit";
    QTest::newRow("^exit\\n") << "^exit\n" << 1 << false << false << 5 << "exit";
    QTest::newRow("^exit\\r\\n") << "^exit\r\n" << 1 << false << false << 5 << "exit";
    QTest::newRow("^__exit") << "^  exit" << 1 << false << false << 7 << "exit";
    QTest::newRow("^stopped,reason") << "^stopped,reason" << 1 << false << false << 8 << "stopped";
    QTest::newRow("_") << "" << 0 << true << true << -1 << "";
    QTest::newRow("^running\\n*running,id=1") << "^running\n*running,id=1" << 1 << false << false << 8 << "running";
}

void TestGdbmi::tryVariable()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(QString, value);

    const auto tok = gdbmi::tryVariable(msg.toUtf8(), start);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QVERIFY(tok.value.has_value());
        QCOMPARE(tok.value.value(), value);
        QCOMPARE(tok.position, position);
    }
}

void TestGdbmi::tryVariable_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<QString>("value");

    QTest::newRow("reason=\"breakpoint-hit\"") << "reason=\"breakpoint-hit\"" << 0 << false << false << 7 << "reason";
    QTest::newRow("reason =\"breakpoint-hit\"") << "reason =\"breakpoint-hit\"" << 0 << false << false << 8 << "reason";
    QTest::newRow("__reason=\"breakpoint-hit\"") << "  reason=\"breakpoint-hit\"" << 0 << false << false << 9 << "reason";
    QTest::newRow("reason") << "reason" << 0 << true << true << 0 << "";
}

void TestGdbmi::tryStreamOutput()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(QString, value);
    QFETCH(char, prefix);

    const auto tok = gdbmi::tryStreamOutput(prefix, msg.toUtf8(), start);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QVERIFY(tok.value.has_value());
        QCOMPARE(tok.value.value().message, value);
        QCOMPARE(tok.position, position);
    }
}

void TestGdbmi::tryStreamOutput_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<QString>("value");
    QTest::addColumn<char>("prefix");

    QTest::newRow("~\"algo\",^running") << "~\"algo\",^running" << 0 << false << false << 7 << "algo" << '~';
    QTest::newRow("~\"algo\"") << "~\"algo\"" << 0 << false << false << 7 << "algo" << '~';
    QTest::newRow("~\"algo\"\\n") << "~\"algo\"\n" << 0 << false << false << 8 << "algo" << '~';
    QTest::newRow("~\"algo\"\\n") << "~\"algo\"\\n" << 0 << false << false << 7 << "algo" << '~';
    QTest::newRow("~\"algo") << "~\"algo" << 0 << false << false << 6 << "\"algo" << '~';
    QTest::newRow("~\"algo\\n") << "~\"algo\n" << 0 << false << false << 7 << "\"algo" << '~';
}

void TestGdbmi::tryResult()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(QJsonObject, value);

    const auto tok = gdbmi::tryResult(msg.toUtf8(), start);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QCOMPARE(tok.position, position);
        QVERIFY(tok.value.has_value());
        const auto tokval = tok.value.value();
        QVERIFY(value.contains(tokval.name));
        compare(value[tokval.name], tokval.value);
    }
}

void TestGdbmi::tryResult_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<QJsonObject>("value");

    QTest::newRow("reason=\"breakpoint-hit\"") << "reason=\"breakpoint-hit\"" << 0 << false << false << 23
                                               << QJsonObject{{QStringLiteral("reason"), QStringLiteral("breakpoint-hit")}};
    QTest::newRow("reason=\"breakpoint-hit") << "reason=\"breakpoint-hit" << 0 << true << true << 0 << QJsonObject{};
    QTest::newRow("^done,reason=\"breakpoint-hit\"") << "^done,reason=\"breakpoint-hit\"" << 6 << false << false << 29
                                                     << QJsonObject{{QStringLiteral("reason"), QStringLiteral("breakpoint-hit")}};
    QTest::newRow("thread-groups=[\"i1\"]") << "thread-groups=[\"i1\"]" << 0 << false << false << 20
                                            << QJsonObject{{QStringLiteral("thread-groups"), QJsonArray{QStringLiteral("i1")}}};
    QTest::newRow("args=[]") << "args=[]" << 0 << false << false << 7 << QJsonObject{{QStringLiteral("args"), QJsonArray()}};
    QTest::newRow("frame={}") << "frame={}" << 0 << false << false << 8 << QJsonObject{{QStringLiteral("frame"), QJsonObject()}};
}

void TestGdbmi::tryResults()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(QJsonObject, value);

    const auto tok = gdbmi::tryResults(msg.toUtf8(), start);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QCOMPARE(tok.position, position);
        QVERIFY(tok.value.has_value());
        compare(value, tok.value.value());
    }
}

void TestGdbmi::tryResults_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<QJsonObject>("value");

    QTest::newRow("string1") << "reason=\"breakpoint-hit\"" << 0 << false << false << 23
                             << QJsonObject{{QStringLiteral("reason"), QStringLiteral("breakpoint-hit")}};
    QTest::newRow("breakpoint1")
        << "number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x08048564\",func=\"main\",file=\"myprog.c\",fullname=\"/home/nickrob/"
           "myprog.c\",line=\"68\",thread-groups=[\"i1\"],times=\"0\""
        << 0 << false << false << 173
        << QJsonObject{{QStringLiteral("number"), QStringLiteral("1")},
                       {QStringLiteral("type"), QStringLiteral("breakpoint")},
                       {QStringLiteral("disp"), QStringLiteral("keep")},
                       {QStringLiteral("enabled"), QStringLiteral("y")},
                       {QStringLiteral("addr"), QStringLiteral("0x08048564")},
                       {QStringLiteral("func"), QStringLiteral("main")},
                       {QStringLiteral("file"), QStringLiteral("myprog.c")},
                       {QStringLiteral("fullname"), QStringLiteral("/home/nickrob/myprog.c")},
                       {QStringLiteral("line"), QStringLiteral("68")},
                       {QStringLiteral("thread-groups"), QJsonArray{QStringLiteral("i1")}},
                       {QStringLiteral("times"), QStringLiteral("0")}};
    QTest::newRow("breakpoint2")
        << "number=\"1\",type=\"breakpoint\",disp=\"del\",enabled=\"y\",addr=\"0x00005555555551f5\",func=\"main()\",file=\"/pruebas/cpp1/"
           "main.cpp\",fullname=\"/pruebas/cpp1/main.cpp\",line=\"7\",thread-groups=[\"i1\"],times=\"1\",original-location=\"main\""
        << 0 << false << false << 220
        << QJsonObject{{QStringLiteral("number"), QStringLiteral("1")},
                       {QStringLiteral("type"), QStringLiteral("breakpoint")},
                       {QStringLiteral("disp"), QStringLiteral("del")},
                       {QStringLiteral("enabled"), QStringLiteral("y")},
                       {QStringLiteral("addr"), QStringLiteral("0x00005555555551f5")},
                       {QStringLiteral("func"), QStringLiteral("main()")},
                       {QStringLiteral("file"), QStringLiteral("/pruebas/cpp1/main.cpp")},
                       {QStringLiteral("fullname"), QStringLiteral("/pruebas/cpp1/main.cpp")},
                       {QStringLiteral("line"), QStringLiteral("7")},
                       {QStringLiteral("thread-groups"), QJsonArray{QStringLiteral("i1")}},
                       {QStringLiteral("times"), QStringLiteral("1")},
                       {QStringLiteral("original-location"), QStringLiteral("main")}};

    QTest::newRow("breakpoint-frame-noargs") << "addr=\"0x00005555555551f5\",func=\"main\",args=[],file=\"/pruebas/cpp1/main.cpp\"" << 0 << false << false << 75
                                             << QJsonObject{{QStringLiteral("addr"), QStringLiteral("0x00005555555551f5")},
                                                            {QStringLiteral("func"), QStringLiteral("main")},
                                                            {QStringLiteral("args"), QJsonArray()},
                                                            {QStringLiteral("file"), QStringLiteral("/pruebas/cpp1/main.cpp")}};

    QTest::newRow("breakpoint-frame-noargs-full") << "addr=\"0x00005555555551f5\",func=\"main\",args=[],file=\"/pruebas/cpp1/main.cpp\",fullname=\"/pruebas/"
                                                     "cpp1/main.cpp\",line=\"7\",arch=\"i386:x86-64\""
                                                  << 0 << false << false << 137
                                                  << QJsonObject{{QStringLiteral("addr"), QStringLiteral("0x00005555555551f5")},
                                                                 {QStringLiteral("func"), QStringLiteral("main")},
                                                                 {QStringLiteral("args"), QJsonArray()},
                                                                 {QStringLiteral("file"), QStringLiteral("/pruebas/cpp1/main.cpp")},
                                                                 {QStringLiteral("fullname"), QStringLiteral("/pruebas/cpp1/main.cpp")},
                                                                 {QStringLiteral("line"), QStringLiteral("7")},
                                                                 {QStringLiteral("arch"), QStringLiteral("i386:x86-64")}};

    QTest::newRow("breakpoint-full")
        << "reason=\"breakpoint-hit\",disp=\"del\",bkptno=\"1\",frame={addr=\"0x00005555555551f5\",func=\"main\",args=[],file=\"/pruebas/cpp1/"
           "main.cpp\",fullname=\"/pruebas/cpp1/main.cpp\",line=\"7\",arch=\"i386:x86-64\"},thread-id=\"1\",stopped-threads=\"all\",core=\"1\"\n"
        << 0 << false << false << 236
        << QJsonObject{{QStringLiteral("reason"), QStringLiteral("breakpoint-hit")},
                       {QStringLiteral("disp"), QStringLiteral("del")},
                       {QStringLiteral("bkptno"), QStringLiteral("1")},
                       {QStringLiteral("frame"),
                        QJsonObject{{QStringLiteral("addr"), QStringLiteral("0x00005555555551f5")},
                                    {QStringLiteral("func"), QStringLiteral("main")},
                                    {QStringLiteral("args"), QJsonArray()},
                                    {QStringLiteral("file"), QStringLiteral("/pruebas/cpp1/main.cpp")},
                                    {QStringLiteral("fullname"), QStringLiteral("/pruebas/cpp1/main.cpp")},
                                    {QStringLiteral("line"), QStringLiteral("7")},
                                    {QStringLiteral("arch"), QStringLiteral("i386:x86-64")}}},
                       {QStringLiteral("thread-id"), QStringLiteral("1")},
                       {QStringLiteral("stopped-threads"), QStringLiteral("all")},
                       {QStringLiteral("core"), QStringLiteral("1")}};
}

void TestGdbmi::tryTuple()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(QJsonObject, value);

    const auto tok = gdbmi::tryTuple(msg.toUtf8(), start);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QCOMPARE(tok.position, position);
        QVERIFY(tok.value.has_value());
        compare(value, tok.value.value());
    }
}

void TestGdbmi::tryTuple_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<QJsonObject>("value");

    QTest::newRow("{}") << "{}" << 0 << false << false << 2 << QJsonObject();
    QTest::newRow("{ }") << "{ }" << 0 << false << false << 3 << QJsonObject();
    QTest::newRow("{reason=\"breakpoint-hit\"}") << "{reason=\"breakpoint-hit\"}" << 0 << false << false << 25
                                                 << QJsonObject{{QStringLiteral("reason"), QStringLiteral("breakpoint-hit")}};
    QTest::newRow(
        "{number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x08048564\",func=\"main\",file=\"myprog.c\",fullname=\"/home/nickrob/"
        "myprog.c\",line=\"68\",thread-groups=[\"i1\"],times=\"0\"}")
        << "{number=\"1\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x08048564\",func=\"main\",file=\"myprog.c\",fullname=\"/home/nickrob/"
           "myprog.c\",line=\"68\",thread-groups=[\"i1\"],times=\"0\"}"
        << 0 << false << false << 175
        << QJsonObject{{QStringLiteral("number"), QStringLiteral("1")},
                       {QStringLiteral("type"), QStringLiteral("breakpoint")},
                       {QStringLiteral("disp"), QStringLiteral("keep")},
                       {QStringLiteral("enabled"), QStringLiteral("y")},
                       {QStringLiteral("addr"), QStringLiteral("0x08048564")},
                       {QStringLiteral("func"), QStringLiteral("main")},
                       {QStringLiteral("file"), QStringLiteral("myprog.c")},
                       {QStringLiteral("fullname"), QStringLiteral("/home/nickrob/myprog.c")},
                       {QStringLiteral("line"), QStringLiteral("68")},
                       {QStringLiteral("thread-groups"), QJsonArray{QStringLiteral("i1")}},
                       {QStringLiteral("times"), QStringLiteral("0")}};
}

void TestGdbmi::tryValue()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(QJsonValue, value);

    const auto tok = gdbmi::tryValue(msg.toUtf8(), start);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QCOMPARE(tok.position, position);
        QVERIFY(tok.value.has_value());
        compare(value, tok.value.value());
    }
}

void TestGdbmi::tryValue_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<QJsonValue>("value");

    QTest::newRow("\"breakpoint-hit\"") << "\"breakpoint-hit\"" << 0 << false << false << 16 << QJsonValue(QStringLiteral("breakpoint-hit"));
    QTest::newRow("{reason=\"breakpoint-hit\"}") << "{reason=\"breakpoint-hit\"}" << 0 << false << false << 25
                                                 << QJsonValue(QJsonObject{{QStringLiteral("reason"), QStringLiteral("breakpoint-hit")}});
    QTest::newRow("[\"breakpoint-hit\", \"i1\"]") << "[\"breakpoint-hit\", \"i1\"]" << 0 << false << false << 24
                                                  << QJsonValue(QJsonArray{QStringLiteral("breakpoint-hit"), QStringLiteral("i1")});
}

void TestGdbmi::tryList()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(QJsonValue, value);

    const auto tok = gdbmi::tryList(msg.toUtf8(), start);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QCOMPARE(tok.position, position);
        QVERIFY(tok.value.has_value());
        compare(value, tok.value.value());
    }
}

void TestGdbmi::tryList_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<QJsonValue>("value");

    QTest::newRow("empty") << "[]" << 0 << false << false << 2 << QJsonValue(QJsonArray());
    QTest::newRow("empty_with_blank") << "[ ]" << 0 << false << false << 3 << QJsonValue(QJsonArray());
    QTest::newRow("single_str_item") << "[\"breakpoint-hit\"]" << 0 << false << false << 18 << QJsonValue(QJsonArray{QStringLiteral("breakpoint-hit")});
    QTest::newRow("item_collection") << "[\"breakpoint-hit\", \"i1\"]" << 0 << false << false << 24
                                     << QJsonValue(QJsonArray{QStringLiteral("breakpoint-hit"), QStringLiteral("i1")});
    QTest::newRow("single_tuple") << "[reason=\"breakpoint-hit\"]" << 0 << false << false << 25
                                  << QJsonValue(QJsonArray{QJsonObject{{QStringLiteral("reason"), QStringLiteral("breakpoint-hit")}}});
    QTest::newRow("single_object") << "[{key1=\"value1\", key2=\"value2\"}]" << 0 << false << false << 32
                                   << QJsonValue(QJsonArray{
                                          QJsonObject{{QStringLiteral("key1"), QStringLiteral("value1")}, {QStringLiteral("key2"), QStringLiteral("value2")}}});
    QTest::newRow("tuple_collection") << "[frame=\"frame1\", frame=\"frame2\"]" << 0 << false << false << 32
                                      << QJsonValue(QJsonArray{QJsonObject{{QStringLiteral("frame"), QStringLiteral("frame1")}},
                                                               QJsonObject{{QStringLiteral("frame"), QStringLiteral("frame2")}}});
}

void TestGdbmi::quoted()
{
    QFETCH(QString, original);
    QFETCH(QString, expected);

    const auto replaced = gdbmi::quotedString(original);

    QCOMPARE(replaced, expected);
}

void TestGdbmi::quoted_data()
{
    QTest::addColumn<QString>("original");
    QTest::addColumn<QString>("expected");

    QTest::newRow("nothing") << "nothing"
                             << "nothing";
    QTest::newRow("quotable") << "abc \"cde\""
                              << "abc \\\"cde\\\"";
    QTest::newRow("already_quoted") << "abc \\\"cde\\\""
                                    << "abc \\\"cde\\\"";
}

void TestGdbmi::tryRecord()
{
    QFETCH(QString, msg);
    QFETCH(int, start);
    QFETCH(bool, empty);
    QFETCH(bool, error);
    QFETCH(int, position);
    QFETCH(int, category);
    QFETCH(QString, resultClass);
    QFETCH(int, token);
    QFETCH(QJsonObject, value);

    const auto tok = gdbmi::tryRecord(msg.toUtf8().at(start), msg.toUtf8(), start, token);

    QCOMPARE(tok.isEmpty(), empty);
    if (empty) {
        QCOMPARE(tok.hasError(), error);
        QCOMPARE(tok.position, start);
    } else {
        QCOMPARE(tok.position, position);
        QVERIFY(tok.value.has_value());
        const auto record = tok.value.value();
        QCOMPARE(category, (int)record.category);
        QCOMPARE(record.token.value_or(-1), token);
        QCOMPARE(resultClass, record.resultClass);
        compare(value, record.value);
    }
}

void TestGdbmi::tryRecord_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("start");
    QTest::addColumn<bool>("empty");
    QTest::addColumn<bool>("error");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("category");
    QTest::addColumn<QString>("resultClass");
    QTest::addColumn<int>("token");
    QTest::addColumn<QJsonObject>("value");

    QTest::newRow("^running") << "^running" << 0 << false << false << 8 << (int)gdbmi::Record::Result << QStringLiteral("running") << -1 << QJsonObject();
    QTest::newRow("123^done") << "123^done" << 3 << false << false << 8 << (int)gdbmi::Record::Result << QStringLiteral("done") << 123 << QJsonObject();
    QTest::newRow("*stopped,reason=\"exited-normally\"")
        << "*stopped,reason=\"exited-normally\"" << 0 << false << false << 33 << (int)gdbmi::Record::Exec << QStringLiteral("stopped") << -1
        << QJsonObject{{QStringLiteral("reason"), QStringLiteral("exited-normally")}};
    QTest::newRow("=thread-created,id=\"id\",group-id=\"gid\"")
        << "=thread-created,id=\"id\",group-id=\"gid\"" << 0 << false << false << 38 << (int)gdbmi::Record::Notify << QStringLiteral("thread-created") << -1
        << QJsonObject{{QStringLiteral("id"), QStringLiteral("id")}, {QStringLiteral("group-id"), QStringLiteral("gid")}};
    QTest::newRow(
        "*stopped,reason=\"breakpoint-hit\",disp=\"keep\",bkptno=\"1\",thread-id=\"0\",frame={addr=\"0x08048564\",func=\"main\",args=[{name=\"argc\",value="
        "\"1\"},{name=\"argv\",value=\"0xbfc4d4d4\"}],file=\"myprog.c\",fullname=\"/home/nickrob/myprog.c\",line=\"68\",arch=\"i386:x86_64\"}")
        << "*stopped,reason=\"breakpoint-hit\",disp=\"keep\",bkptno=\"1\",thread-id=\"0\",frame={addr=\"0x08048564\",func=\"main\",args=[{name=\"argc\",value="
           "\"1\"},{name=\"argv\",value=\"0xbfc4d4d4\"}],file=\"myprog.c\",fullname=\"/home/nickrob/myprog.c\",line=\"68\",arch=\"i386:x86_64\"}"
        << 0 << false << false << 250 << (int)gdbmi::Record::Exec << QStringLiteral("stopped") << -1
        << QJsonObject{{QStringLiteral("reason"), QStringLiteral("breakpoint-hit")},
                       {QStringLiteral("disp"), QStringLiteral("keep")},
                       {QStringLiteral("bkptno"), QStringLiteral("1")},
                       {QStringLiteral("thread-id"), QStringLiteral("0")},
                       {QStringLiteral("frame"),
                        QJsonObject{{QStringLiteral("addr"), QStringLiteral("0x08048564")},
                                    {QStringLiteral("func"), QStringLiteral("main")},
                                    {QStringLiteral("args"),
                                     QJsonArray{QJsonObject{{QStringLiteral("name"), QStringLiteral("argc")}, {QStringLiteral("value"), QStringLiteral("1")}},
                                                QJsonObject{{QStringLiteral("name"), QStringLiteral("argv")},
                                                            {QStringLiteral("value"), QStringLiteral("0xbfc4d4d4")}}}},
                                    {QStringLiteral("file"), QStringLiteral("myprog.c")},
                                    {QStringLiteral("fullname"), QStringLiteral("/home/nickrob/myprog.c")},
                                    {QStringLiteral("line"), QStringLiteral("68")},
                                    {QStringLiteral("arch"), QStringLiteral("i386:x86_64")}}}};
}

void TestGdbmi::parseResponse()
{
    gdbmi::GdbmiParser parser;

    qRegisterMetaType<gdbmi::StreamOutput>("StreamOutput");
    qRegisterMetaType<gdbmi::Record>("Record");
    QSignalSpy outputSpy(&parser, &gdbmi::GdbmiParser::outputProduced);
    QVERIFY(outputSpy.isValid());
    QSignalSpy recSpy(&parser, &gdbmi::GdbmiParser::recordProduced);
    QVERIFY(recSpy.isValid());

    const QString text = QStringLiteral(R"--(
~algo
=thread-created,id="id",group-id="gid"
*stopped,reason="breakpoint-hit",disp="keep",bkptno="1",thread-id="0",frame={addr="0x08048564",func="main",args=[{name="argc",value="1"},{name="argv",value="0xbfc4d4d4"}],file="myprog.c",fullname="/home/nickrob/myprog.c",line="68",arch="i386:x86_64"}
123^done,bkpt={number="1",type="breakpoint",disp="keep",enabled="y",addr="0x08048564",func="main",file="myprog.c",fullname="/home/nickrob/myprog.c",line="68",thread-groups=["i1"],times="0"}
    )--");

    parser.parseResponse(text.toLocal8Bit());

    QCOMPARE(outputSpy.size(), 1);
    QCOMPARE(recSpy.size(), 3);

    QCOMPARE(QStringLiteral("algo"), qvariant_cast<gdbmi::StreamOutput>(outputSpy[0][0]).message);

    QCOMPARE(QStringLiteral("thread-created"), qvariant_cast<gdbmi::Record>(recSpy[0][0]).resultClass);
    QCOMPARE(QStringLiteral("stopped"), qvariant_cast<gdbmi::Record>(recSpy[1][0]).resultClass);
    QCOMPARE(QStringLiteral("done"), qvariant_cast<gdbmi::Record>(recSpy[2][0]).resultClass);
}

void TestGdbmi::parseResponse2()
{
    gdbmi::GdbmiParser parser;

    qRegisterMetaType<gdbmi::StreamOutput>("StreamOutput");
    qRegisterMetaType<gdbmi::Record>("Record");
    QSignalSpy outputSpy(&parser, &gdbmi::GdbmiParser::outputProduced);
    QVERIFY(outputSpy.isValid());
    QSignalSpy recSpy(&parser, &gdbmi::GdbmiParser::recordProduced);
    QVERIFY(recSpy.isValid());

    const QString text = QStringLiteral(R"--(
~"Starting program: /home/nobody/pruebas/build-cpp1/cpp1 \n"
=thread-group-started,id="i1",pid="5271"
=thread-created,id="1",group-id="i1"
=library-loaded,id="/lib64/ld-linux-x86-64.so.2",target-name="/lib64/ld-linux-x86-64.so.2",host-name="/lib64/ld-linux-x86-64.so.2",symbols-loaded="0",thread-group="i1",ranges=[{from="0x00007ffff7fc5090",to="0x00007ffff7fee335"}]
^running
*running,thread-id="all"
(gdb)
)--");

    parser.parseResponse(text.toLocal8Bit());

    QCOMPARE(outputSpy.size(), 1);
    QCOMPARE(recSpy.size(), 6);

    QCOMPARE(QStringLiteral("Starting program: /home/nobody/pruebas/build-cpp1/cpp1 \n"), qvariant_cast<gdbmi::StreamOutput>(outputSpy[0][0]).message);

    QCOMPARE(QStringLiteral("thread-group-started"), qvariant_cast<gdbmi::Record>(recSpy[0][0]).resultClass);
    QCOMPARE(QStringLiteral("thread-created"), qvariant_cast<gdbmi::Record>(recSpy[1][0]).resultClass);
    QCOMPARE(QStringLiteral("library-loaded"), qvariant_cast<gdbmi::Record>(recSpy[2][0]).resultClass);
    QCOMPARE(QStringLiteral("running"), qvariant_cast<gdbmi::Record>(recSpy[3][0]).resultClass);
    QCOMPARE(gdbmi::Record::Result, qvariant_cast<gdbmi::Record>(recSpy[3][0]).category);
    QCOMPARE(QStringLiteral("running"), qvariant_cast<gdbmi::Record>(recSpy[4][0]).resultClass);
    QCOMPARE(gdbmi::Record::Exec, qvariant_cast<gdbmi::Record>(recSpy[4][0]).category);
    QCOMPARE(gdbmi::Record::Prompt, qvariant_cast<gdbmi::Record>(recSpy[5][0]).category);
}

void TestGdbmi::parseResponse3()
{
    gdbmi::GdbmiParser parser;

    qRegisterMetaType<gdbmi::StreamOutput>("StreamOutput");
    qRegisterMetaType<gdbmi::Record>("Record");
    QSignalSpy outputSpy(&parser, &gdbmi::GdbmiParser::outputProduced);
    QVERIFY(outputSpy.isValid());
    QSignalSpy recSpy(&parser, &gdbmi::GdbmiParser::recordProduced);
    QVERIFY(recSpy.isValid());
    QSignalSpy errorSpy(&parser, &gdbmi::GdbmiParser::parserError);
    QVERIFY(errorSpy.isValid());

    const QString text = QStringLiteral(R"--(
=breakpoint-modified,bkpt={number="1",type="breakpoint",disp="del",enabled="y",addr="0x00005555555551f5",func="main()",file="/home/hector/pruebas/cpp1/main.cpp",fullname="/home/hector/pruebas/cpp1/main.cpp",line="7",thread-groups=["i1"],times="1",original-location="main"}
~"\n"
~"Temporary breakpoint 1, main () at /home/hector/pruebas/cpp1/main.cpp:7\n"
~"7\t{\n"
*stopped,reason="breakpoint-hit",disp="del",bkptno="1",frame={addr="0x00005555555551f5",func="main",args=[],file="/home/hector/pruebas/cpp1/main.cpp",fullname="/home/hector/pruebas/cpp1/main.cpp",line="7",arch="i386:x86-64"},thread-id="1",stopped-threads="all",core="1"
=breakpoint-deleted,id="1"
(gdb)
)--");

    parser.parseResponse(text.toLocal8Bit());

    QCOMPARE(errorSpy.size(), 0);
    QCOMPARE(outputSpy.size(), 3);

    QCOMPARE(QStringLiteral("\n"), qvariant_cast<gdbmi::StreamOutput>(outputSpy[0][0]).message);
    QCOMPARE(QStringLiteral("Temporary breakpoint 1, main () at /home/hector/pruebas/cpp1/main.cpp:7\n"),
             qvariant_cast<gdbmi::StreamOutput>(outputSpy[1][0]).message);
    QCOMPARE(QStringLiteral("7\t{\n"), qvariant_cast<gdbmi::StreamOutput>(outputSpy[2][0]).message);

    QCOMPARE(recSpy.size(), 4);

    QCOMPARE(QStringLiteral("breakpoint-modified"), qvariant_cast<gdbmi::Record>(recSpy[0][0]).resultClass);
    QCOMPARE(QStringLiteral("stopped"), qvariant_cast<gdbmi::Record>(recSpy[1][0]).resultClass);
    QCOMPARE(QStringLiteral("breakpoint-deleted"), qvariant_cast<gdbmi::Record>(recSpy[2][0]).resultClass);
    QCOMPARE(gdbmi::Record::Prompt, qvariant_cast<gdbmi::Record>(recSpy[3][0]).category);
}

void TestGdbmi::compare(const QJsonValue &ref, const QJsonValue &result)
{
    if (ref.isNull()) {
        QVERIFY(result.isNull());
    } else if (ref.isUndefined()) {
        QVERIFY(result.isUndefined());
    } else if (ref.isString()) {
        QVERIFY(result.isString());
        QCOMPARE(ref.toString(), result.toString());
    } else if (ref.isBool()) {
        QVERIFY(result.isBool());
        QCOMPARE(ref.toBool(), result.toBool());
    } else if (ref.isDouble()) {
        QVERIFY(result.isDouble());
        QCOMPARE(ref.toDouble(), result.toDouble());
    } else if (ref.isArray()) {
        QVERIFY(result.isArray());
        compare(ref.toArray(), result.toArray());
    } else if (ref.isObject()) {
        QVERIFY(result.isObject());
        compare(ref.toObject(), result.toObject());
    }
}

void TestGdbmi::compare(const QJsonArray &ref, const QJsonArray &result)
{
    QCOMPARE(ref.size(), result.size());

    for (int idx = 0; idx < ref.size(); ++idx) {
        compare(ref.at(idx), result.at(idx));
    }
}

void TestGdbmi::compare(const QJsonObject &ref, const QJsonObject &result)
{
    QCOMPARE(ref.size(), result.size());

    for (auto it = ref.constBegin(); it != ref.constEnd(); ++it) {
        compare(it.value(), result[it.key()]);
    }
}

#include "moc_test_gdbmi.cpp"
