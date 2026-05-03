/*
   SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QMap>
#include <QPair>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QString>

/**
 * Type alias for a foreign key reference: (referenced table, referenced column).
 */
using ColumnReference = QPair<QString, QString>;

/**
 * Type alias for foreign keys of a single table: maps column name to its reference.
 */
using ColumnForeignKeys = QMap<QString, ColumnReference>;

/**
 * Type alias for all foreign keys in a database: maps table name to its column foreign keys.
 */
using DatabaseForeignKeys = QMap<QString, ColumnForeignKeys>;

/**
 * Helper class to retrieve foreign key information from a QSqlDatabase connection.
 *
 * Provides optimized queries for each supported database type, fetching all
 * foreign keys in a single query where possible (e.g., PostgreSQL, MySQL)
 * rather than querying table by table.
 */
class ForeignKeyHelper
{
public:
    /**
     * Retrieves all foreign keys from the given database connection.
     *
     * @param db The database connection to query. Must be valid and open.
     * @return A map of table names to their column foreign keys,
     *         where each column maps to its referenced (table, column) pair.
     *         Returns an empty map if the database is invalid, not open,
     *         or the query fails.
     */
    static DatabaseForeignKeys getForeignKeys(const QSqlDatabase &db);

private:
    /**
     * Inserts a foreign key entry into the result map.
     * Skips entries where any component is empty.
     */
    static void insertForeignKey(DatabaseForeignKeys &result, const QString &table, const QString &column, const QString &refTable, const QString &refColumn);

    /**
     * Retrieves foreign keys for SQLite databases.
     * Uses PRAGMA foreign_key_list for each table.
     */
    static DatabaseForeignKeys getSQLiteForeignKeys(const QSqlDatabase &db);

    /**
     * Retrieves foreign keys for PostgreSQL databases.
     * Uses pg_catalog with LATERAL unnest for correct multi-column FK support.
     */
    static DatabaseForeignKeys getPostgreSQLForeignKeys(const QSqlDatabase &db);

    /**
     * Retrieves foreign keys for MySQL databases.
     * Uses a single query against INFORMATION_SCHEMA.KEY_COLUMN_USAGE.
     */
    static DatabaseForeignKeys getMySqlServerForeignKeys(const QSqlDatabase &db);

    /**
     * Retrieves foreign keys for Microsoft SQL Server databases.
     * Uses sys.foreign_keys and sys.foreign_key_columns catalog views.
     */
    static DatabaseForeignKeys getMSSqlServerForeignKeys(const QSqlDatabase &db);

    /**
     * Retrieves foreign keys for Oracle databases.
     * Uses ALL_CONSTRAINTS and ALL_CONS_COLUMNS with proper owner and position joins.
     */
    static DatabaseForeignKeys getOracleForeignKeys(const QSqlDatabase &db);

    /**
     * Retrieves foreign keys for Sybase/Adaptive Server databases.
     * Returns empty because sysreferences does not expose multi-column
     * foreign keys in a reliably queryable way, making partial results misleading.
     */
    static DatabaseForeignKeys getSybaseForeignKeys(const QSqlDatabase &db);

    /**
     * Retrieves foreign keys for DB2 databases.
     * Uses SYSCAT.REFERENCES joined with SYSCAT.KEYCOLUSE for column detail.
     */
    static DatabaseForeignKeys getDB2ForeignKeys(const QSqlDatabase &db);

    /**
     * Retrieves foreign keys for Interbase/Firebird databases.
     * Uses RDB$RELATION_CONSTRAINTS and RDB$REF_CONSTRAINTS system tables.
     */
    static DatabaseForeignKeys getInterbaseForeignKeys(const QSqlDatabase &db);

    /**
     * Retrieves foreign keys for Mimer SQL databases.
     * Uses INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS with schema and ordinal qualifiers.
     */
    static DatabaseForeignKeys getMimerSQLForeignKeys(const QSqlDatabase &db);

    /**
     * Generic fallback for unknown database types.
     * Returns an empty result as there is no portable way to retrieve foreign keys.
     */
    static DatabaseForeignKeys getUnknownForeignKeys(const QSqlDatabase &db);
};
