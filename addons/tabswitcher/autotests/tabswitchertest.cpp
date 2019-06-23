/* This file is part of the KDE project
 *
 *  Copyright (C) 2018 Gregor Mi <codestruct@posteo.org>
 *  Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "tabswitchertest.h"
#include "tabswitcherfilesmodel.h"

#include <QTest>

QTEST_MAIN(KateTabSwitcherTest)

void KateTabSwitcherTest::initTestCase()
{
}

void KateTabSwitcherTest::cleanupTestCase()
{
}

void KateTabSwitcherTest::testLongestCommonPrefix()
{
    QFETCH(std::vector<QString>, input_strings);
    QFETCH(QString, expected);

    QCOMPARE(detail::longestCommonPrefix(input_strings), expected);
}

void KateTabSwitcherTest::testLongestCommonPrefix_data()
{
    QTest::addColumn<std::vector<QString>>("input_strings");
    QTest::addColumn<QString>("expected");
    std::vector<QString> strs;

    strs.clear();
    strs.push_back(QStringLiteral("/home/user1/a"));
    strs.push_back(QStringLiteral("/home/user1/bc"));
    QTest::newRow("standard case") << strs << QStringLiteral("/home/user1/");

    strs.clear();
    strs.push_back(QStringLiteral("/home/a"));
    strs.push_back(QStringLiteral("/home/b"));
    strs.push_back(QLatin1String(""));
    QTest::newRow("empty string at the end of the list") << strs << QString();

    strs.clear();
    strs.push_back(QLatin1String(""));
    strs.push_back(QStringLiteral("/home/a"));
    strs.push_back(QStringLiteral("/home/b"));
    strs.push_back(QLatin1String(""));
    QTest::newRow("empty string not only at the end of the list") << strs << QString();

    strs.clear();
    strs.push_back(QStringLiteral("/home/a"));
    strs.push_back(QStringLiteral("/etc/b"));
    QTest::newRow("a prefix with length 1") << strs << QStringLiteral("/");

    strs.clear();
    strs.push_back(QStringLiteral("a"));
    strs.push_back(QStringLiteral("a"));
    QTest::newRow("two equal strings") << strs << QStringLiteral("a");

    strs.clear();
    strs.push_back(QStringLiteral("/home/autolink"));
    strs.push_back(QStringLiteral("/home/async"));
    QTest::newRow("find correct path prefix") << strs << QStringLiteral("/home/");
}
