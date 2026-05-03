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
    case QSqlDriver::DbmsType::MSSqlServer:
        return getMSSqlServerForeignKeys(db);
    case QSqlDriver::DbmsType::Oracle:
        return getOracleForeignKeys(db);
    case QSqlDriver::DbmsType::Sybase:
        return getSybaseForeignKeys(db);
    case QSqlDriver::DbmsType::DB2:
        return getDB2ForeignKeys(db);
    case QSqlDriver::DbmsType::Interbase:
        return getInterbaseForeignKeys(db);
    case QSqlDriver::DbmsType::MimerSQL:
        return getMimerSQLForeignKeys(db);
    case QSqlDriver::DbmsType::UnknownDbms:
    default:
        return getUnknownForeignKeys(db);
    }
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
    QStringList tables = db.tables();

    DatabaseForeignKeys result;
    for (const QString &table : tables) {
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
    static const QLatin1String query(
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
    static const QLatin1String query(
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

DatabaseForeignKeys ForeignKeyHelper::getMSSqlServerForeignKeys(const QSqlDatabase &db)
{
    DatabaseForeignKeys result;

    // Single query using SQL Server system catalog views
    static const QLatin1String query(
        "SELECT "
        "    tp.name AS table_name, "
        "    cp.name AS column_name, "
        "    tr.name AS referenced_table, "
        "    cr.name AS referenced_column "
        "FROM "
        "    sys.foreign_keys fk "
        "    INNER JOIN sys.foreign_key_columns fkc ON fk.object_id = fkc.constraint_object_id "
        "    INNER JOIN sys.tables tp ON fkc.parent_object_id = tp.object_id "
        "    INNER JOIN sys.columns cp ON fkc.parent_object_id = cp.object_id AND fkc.parent_column_id = cp.column_id "
        "    INNER JOIN sys.tables tr ON fkc.referenced_object_id = tr.object_id "
        "    INNER JOIN sys.columns cr ON fkc.referenced_object_id = cr.object_id AND fkc.referenced_column_id = cr.column_id");

    QSqlQuery q(db);
    if (!q.exec(query)) {
        qWarning() << "ForeignKeyHelper::getMSSqlServerForeignKeys: query failed:" << q.lastError().text();
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

DatabaseForeignKeys ForeignKeyHelper::getOracleForeignKeys(const QSqlDatabase &db)
{
    DatabaseForeignKeys result;

    // Uses USER_* views for the FK side (current user's tables only) and
    // ALL_CONS_COLUMNS for the referenced side (may be in a different schema).
    // Position matching ensures correct pairing of multi-column FKs.
    static const QLatin1String query(
        "SELECT "
        "    a.table_name, "
        "    a.column_name, "
        "    b.table_name AS referenced_table, "
        "    b.column_name AS referenced_column "
        "FROM "
        "    user_cons_columns a "
        "    INNER JOIN user_constraints c "
        "        ON a.constraint_name = c.constraint_name "
        "    INNER JOIN all_cons_columns b "
        "        ON b.owner = c.r_owner "
        "        AND b.constraint_name = c.r_constraint_name "
        "        AND a.position = b.position "
        "WHERE "
        "    c.constraint_type = 'R'");

    QSqlQuery q(db);
    if (!q.exec(query)) {
        qWarning() << "ForeignKeyHelper::getOracleForeignKeys: query failed:" << q.lastError().text();
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

DatabaseForeignKeys ForeignKeyHelper::getSybaseForeignKeys(const QSqlDatabase &db)
{
    DatabaseForeignKeys result;

    // // Use sysreferences which is available in Sybase ASE.
    // // Note: fkey1/refkey1 only captures the first column of multi-column foreign keys.
    // static const QLatin1String query(
    //     "SELECT "
    //     "    OBJECT_NAME(sr.parentid) AS table_name, "
    //     "    COL_NAME(sr.parentid, sr.fkey1) AS column_name, "
    //     "    OBJECT_NAME(sr.reftabid) AS referenced_table, "
    //     "    COL_NAME(sr.reftabid, sr.refkey1) AS referenced_column "
    //     "FROM "
    //     "    sysreferences sr");

    // QSqlQuery q(db);
    // if (!q.exec(query)) {
    //     qWarning() << "ForeignKeyHelper::getSybaseForeignKeys: query failed:" << q.lastError().text();
    //     return result;
    // }

    // while (q.next()) {
    //     const QString table = q.value(0).toString();
    //     const QString column = q.value(1).toString();
    //     const QString refTable = q.value(2).toString();
    //     const QString refColumn = q.value(3).toString();
    //     insertForeignKey(result, table, column, refTable, refColumn);
    // }

    // return result;

    // Sybase ASE sysreferences only exposes fkey1/refkey1 etc. as separate columns
    // (not as a normalized set), making correct multi-column FK retrieval unreliable.
    // Return empty to avoid returning incomplete or misleading results.
    Q_UNUSED(db)
    return {};
}

DatabaseForeignKeys ForeignKeyHelper::getDB2ForeignKeys(const QSqlDatabase &db)
{
    DatabaseForeignKeys result;

    // Fixed: SYSCAT.REFERENCES does not contain column names directly.
    // Join with SYSCAT.KEYCOLUSE to get both referencing and referenced columns,
    // matching on COLSEQ for correct multi-column FK pairing.
    static const QLatin1String query(
        "SELECT "
        "    r.TABNAME AS table_name, "
        "    k.COLNAME AS column_name, "
        "    r.REFTABNAME AS referenced_table, "
        "    kr.COLNAME AS referenced_column "
        "FROM "
        "    SYSCAT.REFERENCES r "
        "    INNER JOIN SYSCAT.KEYCOLUSE k "
        "        ON r.CONSTNAME = k.CONSTNAME "
        "        AND r.TABSCHEMA = k.TABSCHEMA "
        "        AND r.TABNAME = k.TABNAME "
        "    INNER JOIN SYSCAT.KEYCOLUSE kr "
        "        ON r.REFKEYNAME = kr.CONSTNAME "
        "        AND r.REFSCHEMA = kr.TABSCHEMA "
        "        AND kr.COLSEQ = k.COLSEQ "
        "WHERE "
        "    r.TABSCHEMA = CURRENT_SCHEMA");

    QSqlQuery q(db);
    if (!q.exec(query)) {
        qWarning() << "ForeignKeyHelper::getDB2ForeignKeys: query failed:" << q.lastError().text();
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

DatabaseForeignKeys ForeignKeyHelper::getInterbaseForeignKeys(const QSqlDatabase &db)
{
    DatabaseForeignKeys result;

    // Query using Firebird/Interbase system tables
    static const QLatin1String query(
        "SELECT "
        "    rc.rdb$relation_name AS table_name, "
        "    s.rdb$field_name AS column_name, "
        "    ref.rdb$relation_name AS referenced_table, "
        "    refs.rdb$field_name AS referenced_column "
        "FROM "
        "    rdb$relation_constraints rc "
        "    INNER JOIN rdb$ref_constraints refc ON rc.rdb$constraint_name = refc.rdb$constraint_name "
        "    INNER JOIN rdb$index_segments s ON rc.rdb$index_name = s.rdb$index_name "
        "    INNER JOIN rdb$relation_constraints ref ON refc.rdb$const_name_uq = ref.rdb$constraint_name "
        "    INNER JOIN rdb$index_segments refs ON ref.rdb$index_name = refs.rdb$index_name "
        "WHERE "
        "    rc.rdb$constraint_type = 'FOREIGN KEY' "
        "    AND s.rdb$field_position = refs.rdb$field_position");

    QSqlQuery q(db);
    if (!q.exec(query)) {
        qWarning() << "ForeignKeyHelper::getInterbaseForeignKeys: query failed:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        // Firebird returns field names with trailing spaces
        const QString table = q.value(0).toString().trimmed();
        const QString column = q.value(1).toString().trimmed();
        const QString refTable = q.value(2).toString().trimmed();
        const QString refColumn = q.value(3).toString().trimmed();
        insertForeignKey(result, table, column, refTable, refColumn);
    }

    return result;
}

DatabaseForeignKeys ForeignKeyHelper::getMimerSQLForeignKeys(const QSqlDatabase &db)
{
    DatabaseForeignKeys result;

    // Fixed: added constraint_schema qualifiers to avoid cross-schema constraint
    // name collisions, and ordinal_position matching for multi-column FK pairing.
    static const QLatin1String query(
        "SELECT "
        "    kcu.table_name, "
        "    kcu.column_name, "
        "    kcu_referred.table_name AS referenced_table, "
        "    kcu_referred.column_name AS referenced_column "
        "FROM "
        "    INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS rc "
        "    INNER JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE kcu "
        "        ON rc.constraint_name = kcu.constraint_name "
        "        AND rc.constraint_schema = kcu.constraint_schema "
        "    INNER JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE kcu_referred "
        "        ON rc.unique_constraint_name = kcu_referred.constraint_name "
        "        AND rc.unique_constraint_schema = kcu_referred.constraint_schema "
        "        AND kcu.ordinal_position = kcu_referred.ordinal_position");

    QSqlQuery q(db);
    if (!q.exec(query)) {
        qWarning() << "ForeignKeyHelper::getMimerSQLForeignKeys: query failed:" << q.lastError().text();
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

DatabaseForeignKeys ForeignKeyHelper::getUnknownForeignKeys(const QSqlDatabase &db)
{
    // For unknown database types, return empty result.
    // Qt's QSqlDatabase doesn't provide a portable way to retrieve foreign keys.
    // Individual drivers may support it via native handles, but that's beyond
    // the scope of a generic implementation.
    Q_UNUSED(db)
    return {};
}
