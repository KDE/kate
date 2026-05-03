/*
   SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QMap>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QString>

/**
 * Type alias for enum values of a single column.
 */
using ColumnEnums = QStringList;

/**
 * Type alias for enum columns of a single table: maps column name to its enum values.
 */
using TableEnums = QMap<QString, ColumnEnums>;

/**
 * Type alias for all enum columns in a database: maps table name to its column enums.
 */
using DatabaseEnums = QMap<QString, TableEnums>;

/**
 * Helper class to retrieve enum column values from a QSqlDatabase connection.
 *
 * Provides optimized queries for each supported database type, fetching all
 * enum column definitions in a single query where possible (e.g., PostgreSQL, MySQL).
 *
 * Only databases with native enum types (MySQL ENUM, PostgreSQL user-defined enum types)
 * are supported. For other databases, an empty map is returned.
 */
class EnumHelper
{
public:
    /**
     * Retrieves all enum column values from the given database connection.
     *
     * @param db The database connection to query. Must be valid and open.
     * @return A map of table names to their column enum values,
     *         where each column maps to its list of allowed values.
     *         Returns an empty map if the database is invalid, not open,
     *         or the query fails.
     */
    static DatabaseEnums getEnums(const QSqlDatabase &db);

private:
    /**
     * Retrieves enum column values for SQLite databases.
     * Returns empty because SQLite does not have native enum types.
     */
    static DatabaseEnums getSQLiteEnums(const QSqlDatabase &db);

    /**
     * Retrieves enum column values for PostgreSQL databases.
     * Uses pg_enum joined with pg_type, pg_attribute, and pg_class.
     * Returns schema-qualified table names to match QSqlDatabase::tables() output.
     */
    static DatabaseEnums getPostgreSQLEnums(const QSqlDatabase &db);

    /**
     * Retrieves enum column values for MySQL/MariaDB databases.
     * Uses INFORMATION_SCHEMA.COLUMNS with COLUMN_TYPE parsing.
     */
    static DatabaseEnums getMySqlServerEnums(const QSqlDatabase &db);

    /**
     * Retrieves enum column values for Microsoft SQL Server databases.
     * Returns empty because SQL Server does not have native enum types.
     */
    static DatabaseEnums getMSSqlServerEnums(const QSqlDatabase &db);

    /**
     * Retrieves enum column values for Oracle databases.
     * Returns empty because Oracle does not have native enum types.
     */
    static DatabaseEnums getOracleEnums(const QSqlDatabase &db);

    /**
     * Retrieves enum column values for Sybase databases.
     * Returns empty because Sybase does not have native enum types.
     */
    static DatabaseEnums getSybaseEnums(const QSqlDatabase &db);

    /**
     * Retrieves enum column values for DB2 databases.
     * Returns empty because DB2 does not have native enum types.
     */
    static DatabaseEnums getDB2Enums(const QSqlDatabase &db);

    /**
     * Retrieves enum column values for Interbase/Firebird databases.
     * Returns empty because Firebird does not have native enum types.
     */
    static DatabaseEnums getInterbaseEnums(const QSqlDatabase &db);

    /**
     * Retrieves enum column values for Mimer SQL databases.
     * Returns empty because Mimer SQL does not have native enum types.
     */
    static DatabaseEnums getMimerSQLEnums(const QSqlDatabase &db);

    /**
     * Generic fallback for unknown database types.
     * Returns an empty map.
     */
    static DatabaseEnums getUnknownEnums(const QSqlDatabase &db);
};
