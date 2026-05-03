/*
   SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "enumhelper.h"

#include <QDebug>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

DatabaseEnums EnumHelper::getEnums(const QSqlDatabase &db)
{
    if (!db.isValid() || !db.isOpen()) {
        return {};
    }

    const auto dbmsType = db.driver()->dbmsType();

    switch (dbmsType) {
    case QSqlDriver::DbmsType::SQLite:
        return getSQLiteEnums(db);
    case QSqlDriver::DbmsType::PostgreSQL:
        return getPostgreSQLEnums(db);
    case QSqlDriver::DbmsType::MySqlServer:
        return getMySqlServerEnums(db);
    case QSqlDriver::DbmsType::MSSqlServer:
        return getMSSqlServerEnums(db);
    case QSqlDriver::DbmsType::Oracle:
        return getOracleEnums(db);
    case QSqlDriver::DbmsType::Sybase:
        return getSybaseEnums(db);
    case QSqlDriver::DbmsType::DB2:
        return getDB2Enums(db);
    case QSqlDriver::DbmsType::Interbase:
        return getInterbaseEnums(db);
    case QSqlDriver::DbmsType::MimerSQL:
        return getMimerSQLEnums(db);
    case QSqlDriver::DbmsType::UnknownDbms:
    default:
        return getUnknownEnums(db);
    }
}

DatabaseEnums EnumHelper::getSQLiteEnums(const QSqlDatabase &db)
{
    // SQLite does not have native enum types.
    Q_UNUSED(db)
    return {};
}

DatabaseEnums EnumHelper::getPostgreSQLEnums(const QSqlDatabase &db)
{
    DatabaseEnums result;

    // Query PostgreSQL enum types by joining pg_enum with pg_type, pg_attribute,
    // and pg_class. Returns schema-qualified table names to match
    // QSqlDatabase::tables() output format for PostgreSQL.
    static const QLatin1String query(
        "SELECT "
        "    ns.nspname || '.' || c.relname AS table_name, "
        "    a.attname AS column_name, "
        "    e.enumlabel AS enum_value "
        "FROM "
        "    pg_attribute a "
        "    INNER JOIN pg_class c ON a.attrelid = c.oid "
        "    INNER JOIN pg_namespace ns ON c.relnamespace = ns.oid "
        "    INNER JOIN pg_type t ON a.atttypid = t.oid "
        "    INNER JOIN pg_enum e ON e.enumtypid = t.oid "
        "WHERE "
        "    t.typtype = 'e' "
        "    AND a.attnum > 0 "
        "    AND NOT a.attisdropped "
        "    AND ns.nspname NOT IN ('information_schema', 'pg_catalog', 'pg_toast') "
        "ORDER BY "
        "    ns.nspname, c.relname, a.attname, e.enumsortorder");

    QSqlQuery q(db);
    if (!q.exec(query)) {
        qWarning() << "EnumHelper::getPostgreSQLEnums: query failed:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        const QString table = q.value(0).toString();
        const QString column = q.value(1).toString();
        const QString value = q.value(2).toString();
        if (!table.isEmpty() && !column.isEmpty() && !value.isEmpty()) {
            result[table][column].append(value);
        }
    }

    return result;
}

DatabaseEnums EnumHelper::getMySqlServerEnums(const QSqlDatabase &db)
{
    DatabaseEnums result;

    // Query MySQL ENUM columns from INFORMATION_SCHEMA.COLUMNS.
    // COLUMN_TYPE for enum columns looks like: enum('val1','val2','val3')
    // We parse the values from this string.
    static const QLatin1String query(
        "SELECT "
        "    TABLE_NAME, "
        "    COLUMN_NAME, "
        "    COLUMN_TYPE "
        "FROM "
        "    INFORMATION_SCHEMA.COLUMNS "
        "WHERE "
        "    TABLE_SCHEMA = DATABASE() "
        "    AND DATA_TYPE = 'enum'");

    QSqlQuery q(db);
    if (!q.exec(query)) {
        qWarning() << "EnumHelper::getMySqlServerEnums: query failed:" << q.lastError().text();
        return result;
    }

    // Regular expression to extract values from enum('val1','val2',...)
    static const QRegularExpression enumRegex(QStringLiteral("^enum\\((.*)\\)$"), QRegularExpression::CaseInsensitiveOption);

    // Regular expression to extract individual values, handling escaped quotes
    static const QRegularExpression valueRegex(QStringLiteral("'((?:[^']*|'')*)'"));

    while (q.next()) {
        const QString table = q.value(0).toString();
        const QString column = q.value(1).toString();
        const QString columnType = q.value(2).toString();

        if (table.isEmpty() || column.isEmpty()) {
            continue;
        }

        const auto enumMatch = enumRegex.match(columnType);
        if (!enumMatch.hasMatch()) {
            continue;
        }

        const QString valuesStr = enumMatch.captured(1);
        QStringList values;

        QRegularExpressionMatchIterator it = valueRegex.globalMatch(valuesStr);
        while (it.hasNext()) {
            const QRegularExpressionMatch valueMatch = it.next();
            // Replace double single quotes with a single quote (MySQL escaping)
            values.append(valueMatch.captured(1).replace(QStringLiteral("''"), QStringLiteral("'")));
        }

        if (!values.isEmpty()) {
            result[table][column] = values;
        }
    }

    return result;
}

DatabaseEnums EnumHelper::getMSSqlServerEnums(const QSqlDatabase &db)
{
    // SQL Server does not have native enum types.
    Q_UNUSED(db)
    return {};
}

DatabaseEnums EnumHelper::getOracleEnums(const QSqlDatabase &db)
{
    // Oracle does not have native enum types.
    Q_UNUSED(db)
    return {};
}

DatabaseEnums EnumHelper::getSybaseEnums(const QSqlDatabase &db)
{
    // Sybase does not have native enum types.
    Q_UNUSED(db)
    return {};
}

DatabaseEnums EnumHelper::getDB2Enums(const QSqlDatabase &db)
{
    // DB2 does not have native enum types.
    Q_UNUSED(db)
    return {};
}

DatabaseEnums EnumHelper::getInterbaseEnums(const QSqlDatabase &db)
{
    // Firebird/Interbase does not have native enum types.
    Q_UNUSED(db)
    return {};
}

DatabaseEnums EnumHelper::getMimerSQLEnums(const QSqlDatabase &db)
{
    // Mimer SQL does not have native enum types.
    Q_UNUSED(db)
    return {};
}

DatabaseEnums EnumHelper::getUnknownEnums(const QSqlDatabase &db)
{
    Q_UNUSED(db)
    return {};
}
