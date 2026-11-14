/*
SPDX-FileCopyrightText: 2026 Waqar Ahmed <waqar.ahmed@kdab.com>
SPDX-License-Identifier: LGPL-2.0-only
*/

#include <KConfig>
#include <KConfigGroup>

#include <QFileInfo>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "connectionmodel.h"
#include "katesqlconstants.h"
#include "schemawidget.h"
#include "sqlmanager.h"

class TestSQLManager : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void restoredConnectionIsOpenedLazily();
};

void TestSQLManager::initTestCase()
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        QSKIP("QSQLITE driver not available");
    }
}

// Restoring a session must not connect to the saved databases: a remote server
// would block the main window from appearing (bug 528270). The connection must
// only be opened once it is actually used.
void TestSQLManager::restoredConnectionIsOpenedLazily()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString dbPath = dir.filePath(QStringLiteral("restored.sqlite"));
    const QString name = QStringLiteral("restored");

    KConfig config(QString(), KConfig::SimpleConfig);
    KConfigGroup connections(&config, QStringLiteral("KateSQL"));
    KConfigGroup group = connections.group(name);
    group.writeEntry(KateSQLConstants::Connection::Driver, QStringLiteral("QSQLITE"));
    group.writeEntry(KateSQLConstants::Connection::Database, QUrl::fromLocalFile(dbPath));

    SQLManager manager;
    QSignalSpy created(&manager, &SQLManager::connectionCreated);
    QWidget parent;
    SchemaWidget schema(&parent, &manager);
    connect(&manager, &SQLManager::connectionCreated, &schema, &SchemaWidget::buildTree);

    manager.loadConnections(connections);

    QCOMPARE(created.count(), 1);
    QCOMPARE(created.at(0).at(0).toString(), name);
    QCOMPARE(manager.connectionModel()->rowCount(), 1);
    QCOMPARE(manager.connectionModel()->status(name), Connection::UNKNOWN);

    // Neither loading nor building the schema tree may open the connection
    QVERIFY(QSqlDatabase::contains(name));
    QVERIFY(!QSqlDatabase::database(name, false).isOpen());
    QVERIFY(!QFileInfo::exists(dbPath));
    QCOMPARE(schema.topLevelItemCount(), 1);

    // First use opens it
    QVERIFY(manager.isValidAndOpen(name));
    QVERIFY(QSqlDatabase::database(name, false).isOpen());
    QCOMPARE(manager.connectionModel()->status(name), Connection::ONLINE);
}

QTEST_MAIN(TestSQLManager)

#include "test_sqlmanager.moc"
