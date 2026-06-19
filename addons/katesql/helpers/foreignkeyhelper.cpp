/*
   SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "foreignkeyhelper.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

DatabaseForeignKeys ForeignKeyHelper::getForeignKeys(const QSqlDatabase &db)
{
    if (!db.isValid() || !db.isOpen()) {
        return {};
    }

    const auto dbmsType = db.driver()->dbmsType();

    switch (dbmsType) {
    case QSqlDriver::DbmsType::SQLite:
        return getSQLiteForeignKeys(db);
    case QSqlDriver::DbmsType::PostgreSQL:
        return getPostgreSQLForeignKeys(db);
    case QSqlDriver::DbmsType::MySqlServer:
        return getMySqlServerForeignKeys(db);
    default:
        break;
    }
    return {};
}

void ForeignKeyHelper::insertForeignKey(DatabaseForeignKeys &result,
                                        const QString &table,
                                        const QString &column,
                                        const QString &refTable,
                                        const QString &refColumn)
{
    if (!table.isEmpty() && !column.isEmpty() && !refTable.isEmpty() && !refColumn.isEmpty()) {
        result[table][column] = qMakePair(refTable, refColumn);
    }
}

DatabaseForeignKeys ForeignKeyHelper::getSQLiteForeignKeys(const QSqlDatabase &db)
{
    const QStringList tables = db.tables();

    DatabaseForeignKeys result;
    for (const QString &table : std::as_const(tables)) {
        QSqlQuery fkQuery(db);
        const QString escapedTable = QString(table).replace(QLatin1Char('\''), QStringLiteral("''"));
        if (!fkQuery.exec(QStringLiteral("PRAGMA foreign_key_list('%1')").arg(escapedTable))) {
            qWarning() << "ForeignKeyHelper::getSQLiteForeignKeys: PRAGMA failed for table" << table << ":" << fkQuery.lastError().text();
            continue;
        }

        while (fkQuery.next()) {
            const QSqlRecord rec = fkQuery.record();
            const QString column = rec.value(QStringLiteral("from")).toString();
            const QString refTable = rec.value(QStringLiteral("table")).toString();
            const QString refColumn = rec.value(QStringLiteral("to")).toString();
            insertForeignKey(result, table, column, refTable, refColumn);
        }
    }

    return result;
}

DatabaseForeignKeys ForeignKeyHelper::getPostgreSQLForeignKeys(const QSqlDatabase &db)
{
    DatabaseForeignKeys result;

    // Use pg_catalog for reliable multi-column FK support.
    // LATERAL unnest with WITH ORDINALITY pairs FK columns correctly by ordinal position.
    // Returns schema-qualified table names (e.g., "public.my_table") to match
    // QSqlDatabase::tables() output format for PostgreSQL.
    const QString query = QStringLiteral(
        "SELECT "
        "    ns.nspname || '.' || cl.relname AS table_name, "
        "    att2.attname AS column_name, "
        "    ns2.nspname || '.' || cl2.relname AS foreign_table_name, "
        "    att.attname AS foreign_column_name "
        "FROM "
        "    pg_constraint con "
        "    INNER JOIN pg_class cl ON con.conrelid = cl.oid "
        "    INNER JOIN pg_class cl2 ON con.confrelid = cl2.oid "
        "    INNER JOIN pg_namespace ns ON cl.relnamespace = ns.oid "
        "    INNER JOIN pg_namespace ns2 ON cl2.relnamespace = ns2.oid "
        "    CROSS JOIN LATERAL unnest(con.conkey, con.confkey) WITH ORDINALITY AS cols(key, fkey, ord) "
        "    INNER JOIN pg_attribute att ON att.attrelid = con.confrelid AND att.attnum = cols.fkey "
        "    INNER JOIN pg_attribute att2 ON att2.attrelid = con.conrelid AND att2.attnum = cols.key "
        "WHERE "
        "    con.contype = 'f' "
        "    AND ns.nspname NOT IN ('information_schema', 'pg_catalog', 'pg_toast')");

    QSqlQuery q(db);
    if (!q.exec(query)) {
        qWarning() << "ForeignKeyHelper::getPostgreSQLForeignKeys: query failed:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        const QString table = q.value(0).toString();
        const QString column = q.value(1).toString();
        const QString refTable = q.value(2).toString();
        const QString refColumn = q.value(3).toString();
        insertForeignKey(result, table, column, refTable, refColumn);
    }

    return result;
}

DatabaseForeignKeys ForeignKeyHelper::getMySqlServerForeignKeys(const QSqlDatabase &db)
{
    DatabaseForeignKeys result;

    // Single query to get all foreign keys from INFORMATION_SCHEMA
    // Works for both MySQL and MariaDB
    static const QString query = QStringLiteral(
        "SELECT "
        "    TABLE_NAME, "
        "    COLUMN_NAME, "
        "    REFERENCED_TABLE_NAME, "
        "    REFERENCED_COLUMN_NAME "
        "FROM "
        "    INFORMATION_SCHEMA.KEY_COLUMN_USAGE "
        "WHERE "
        "    REFERENCED_TABLE_SCHEMA = DATABASE() "
        "    AND REFERENCED_TABLE_NAME IS NOT NULL");

    QSqlQuery q(db);
    if (!q.exec(query)) {
        qWarning() << "ForeignKeyHelper::getMySqlServerForeignKeys: query failed:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        const QString table = q.value(0).toString();
        const QString column = q.value(1).toString();
        const QString refTable = q.value(2).toString();
        const QString refColumn = q.value(3).toString();
        insertForeignKey(result, table, column, refTable, refColumn);
    }

    return result;
}
