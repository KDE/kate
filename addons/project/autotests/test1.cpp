/* This file is part of the KDE project
 *
 *  SPDX-FileCopyrightText: 2014 Gregor Mi <codestruct@posteo.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "test1.h"
#include "diagnostics/diagnostic_types.h"
#include "fileutil.h"
#include "kateprojectconfigpage.h"
#include "kateprojectplugin.h"
#include "tools/shellcheck.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QCheckBox>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <QString>

QTEST_MAIN(Test1)

void Test1::initTestCase()
{
}

void Test1::cleanupTestCase()
{
}

void Test1::testCommonParent()
{
    QCOMPARE(FileUtil::commonParent(QStringLiteral("/usr/local/bin"), QStringLiteral("/usr/local/bin")), QStringLiteral("/usr/local/"));
    QCOMPARE(FileUtil::commonParent(QStringLiteral("/usr/local"), QStringLiteral("/usr/local/bin")), QStringLiteral("/usr/"));
    QCOMPARE(FileUtil::commonParent(QStringLiteral("~/dev/proj1"), QStringLiteral("~/dev/proj222")), QStringLiteral("~/dev/"));
}

void Test1::testShellCheckParsing()
{
    QString line = QStringLiteral("script.sh:3:11: note: Use ./*glob* or -- *glob* so ... options. [SC2035]");
    KateProjectCodeAnalysisToolShellcheck sc(nullptr);
    FileDiagnostics outList = sc.parseLine(line);
    // qDebug() << outList;
    QVERIFY(outList.uri.isValid());
}

void Test1::testWindowTitleConfig()
{
    QStandardPaths::setTestModeEnabled(true);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    KConfig::setMainConfigName(dir.filePath(QStringLiteral("katerc")));

    {
        KateProjectPlugin plugin(nullptr);
        QVERIFY(!plugin.prefixWindowTitle());
        KateProjectConfigPage page(nullptr, &plugin);
        auto checkbox = page.findChild<QCheckBox *>(QStringLiteral("prefixWindowTitle"));
        QVERIFY(checkbox);
        QVERIFY(!checkbox->isChecked());

        QSignalSpy changed(&page, &KTextEditor::ConfigPage::changed);
        checkbox->setChecked(true);
        QVERIFY(!changed.isEmpty());
        page.apply();
        QVERIFY(plugin.prefixWindowTitle());
        KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("project"));
        QVERIFY(config.readEntry("prefixWindowTitle", false));

        checkbox->setChecked(false);
        page.reset();
        QVERIFY(checkbox->isChecked());
        page.defaults();
        QVERIFY(!checkbox->isChecked());
        page.apply();
        QVERIFY(!plugin.prefixWindowTitle());
        plugin.setPrefixWindowTitle(true);
    }

    KateProjectPlugin restoredPlugin(nullptr);
    QVERIFY(restoredPlugin.prefixWindowTitle());
    restoredPlugin.setPrefixWindowTitle(false);
}

#include "moc_test1.cpp"

// kate: space-indent on; indent-width 4; replace-tabs on;
