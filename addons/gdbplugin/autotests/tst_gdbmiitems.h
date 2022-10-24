/*
 *  SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KATE_GDB_TEST_GDBMI_H
#define KATE_GDB_TEST_GDBMI_H

#include <QObject>

class TestGdbmiItems : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tryToken();
    void tryToken_data();

    void advanceBlanks();
    void advanceBlanks_data();

    void tryString();
    void tryString_data();

    void tryClassName();
    void tryClassName_data();

    void tryVariable();
    void tryVariable_data();

    void tryStreamOutput();
    void tryStreamOutput_data();

    void tryResult();
    void tryResult_data();

    void tryResults();
    void tryResults_data();

    void tryTuple();
    void tryTuple_data();

    void tryValue();
    void tryValue_data();

    void tryList();
    void tryList_data();

    void tryRecord();
    void tryRecord_data();

    void parseResponse();
    void parseResponse2();
    void parseResponse3();

    void quoted();
    void quoted_data();

    void compare(const QJsonValue &ref, const QJsonValue &result);
    void compare(const QJsonArray &ref, const QJsonArray &result);
    void compare(const QJsonObject &ref, const QJsonObject &result);
};

#endif
