/* This file is part of the KDE project
 *
 *  Copyright (C) 2018 Gregor Mi <codestruct@posteo.org>
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
    // standard case
    std::vector<QString> strs;
    strs.push_back(QLatin1String("/home/user1/a"));
    strs.push_back(QLatin1String("/home/user1/bc"));
    QCOMPARE(detail::longestCommonPrefix(strs), QLatin1String("/home/user1/"));

    // empty string at the end of the list
    strs.clear();
    strs.push_back(QLatin1String("/home/a"));
    strs.push_back(QLatin1String("/home/b"));
    strs.push_back(QLatin1String(""));
    QCOMPARE(detail::longestCommonPrefix(strs), QLatin1String(""));

    // empty string not only at the end of the list
    strs.clear();
    strs.push_back(QLatin1String(""));
    strs.push_back(QLatin1String("/home/a"));
    strs.push_back(QLatin1String("/home/b"));
    strs.push_back(QLatin1String(""));
    QCOMPARE(detail::longestCommonPrefix(strs), QLatin1String(""));

    // a prefix with length 1
    strs.clear();
    strs.push_back(QLatin1String("/home/a"));
    strs.push_back(QLatin1String("/etc/b"));
    QCOMPARE(detail::longestCommonPrefix(strs), QLatin1String("/"));
}
