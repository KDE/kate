/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "bytearraysplitter_tests.h"
#include "bytearraysplitter.h"

#include <QTest>
#include <iostream>

QTEST_MAIN(ByteArraySplitterTests)

void ByteArraySplitterTests::test_data()
{
    const QByteArray b("hello\0world\0waqar", sizeof("hello\0world\0waqar") - 1);

    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<char>("splitOn");

    QTest::addRow("1") << QByteArray("hello\0world\0foo", sizeof("hello\0world\0foo") - 1) << char('\0');
    QTest::addRow("2") << QByteArray("hello\nworld\nfoo") << char('\n');
    QTest::addRow("3") << QByteArray("abc\ndef\nhij") << char('z');
}

void ByteArraySplitterTests::test()
{
    QFETCH(QByteArray, data);
    QFETCH(char, splitOn);

    QByteArrayList actual;
    for (auto sv : ByteArraySplitter(data, splitOn)) {
        actual.append(QByteArray(sv.data(), sv.size()));
    }
    QCOMPARE(data.split(splitOn), actual);
}
