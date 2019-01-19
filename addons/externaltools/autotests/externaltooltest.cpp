/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
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

#include "externaltooltest.h"
#include "../kateexternaltool.h"
#include "../katetoolrunner.h"

#include <QtTest>
#include <QString>

#include <KConfig>
#include <KConfigGroup>

QTEST_MAIN(ExternalToolTest)

void ExternalToolTest::initTestCase()
{
}

void ExternalToolTest::cleanupTestCase()
{
}

void ExternalToolTest::testLoadSave()
{
    KConfig config;
    KConfigGroup cg(&config, "tool");

    KateExternalTool tool;
    tool.name = QStringLiteral("git cola");
    tool.icon = QStringLiteral("git-cola");
    tool.executable = QStringLiteral("git-cola");
    tool.arguments = QStringLiteral("none");
    tool.command = QStringLiteral("git-cola");
    tool.mimetypes = QStringList{ QStringLiteral("everything") };
    tool.hasexec = true;
    tool.actionName = QStringLiteral("asdf");
    tool.cmdname = QStringLiteral("git-cola");
    tool.saveMode = KateExternalTool::SaveMode::None;

    tool.save(cg);

    KateExternalTool copiedTool;
    copiedTool.load(cg);
    QCOMPARE(tool.name, copiedTool.name);
}

void ExternalToolTest::testRunListDirectory()
{
    KateExternalTool tool;
    tool.name = QStringLiteral("ls");
    tool.icon = QStringLiteral("none");
    tool.executable = QStringLiteral("ls");
    tool.arguments = QStringLiteral("/");
    tool.command = QStringLiteral("ls");
    tool.mimetypes = QStringList{};
    tool.hasexec = true;
    tool.actionName = QStringLiteral("ls");
    tool.cmdname = QStringLiteral("ls");
    tool.saveMode = KateExternalTool::SaveMode::None;

    KateToolRunner runner(&tool);
    runner.run();
    runner.waitForFinished();
    qDebug() << runner.stdoutData();
    QVERIFY(!runner.stdoutData().isEmpty());
}

// kate: space-indent on; indent-width 4; replace-tabs on;
