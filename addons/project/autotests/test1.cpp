/* This file is part of the KDE project
 *
 *  Copyright 2014 Gregor Mi <codestruct@posteo.org>
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

#include "test1.h"
#include "fileutil.h"

#include <QtTest>

QTEST_MAIN(Test1)

void Test1::initTestCase()
{
}

void Test1::cleanupTestCase()
{
}

void Test1::testCommonParent()
{
    QCOMPARE(FileUtil::commonParent(QLatin1String("/usr/local/bin"), QLatin1String("/usr/local/bin")), QLatin1String("/usr/local/"));
    QCOMPARE(FileUtil::commonParent(QLatin1String("/usr/local"), QLatin1String("/usr/local/bin")), QLatin1String("/usr/"));
    QCOMPARE(FileUtil::commonParent(QLatin1String("~/dev/proj1"), QLatin1String("~/dev/proj222")), QLatin1String("~/dev/"));
}

// kate: space-indent on; indent-width 4; replace-tabs on;
