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

TableEnums EnumHelper::getEnums(const QSqlDatabase &db, const QString &tableName)
{
    if (!db.isValid() || !db.isOpen()) {
        return {};
    }

    if (tableName.isEmpty()) {
        return {};
    }

    const auto dbmsType = db.driver()->dbmsType();

    switch (dbmsType) {
    case QSqlDriver::DbmsType::PostgreSQL:
        return getPostgreSQLEnums(db, tableName);
    case QSqlDriver::DbmsType::MySqlServer:
        return getMySqlServerEnums(db, tableName);
    default:
        return {};
    }
}

TableEnums EnumHelper::getPostgreSQLEnums(const QSqlDatabase &db, const QString &tableName)
{
    TableEnums result;

    // Query PostgreSQL enum types by joining pg_enum with pg_type, pg_attribute,
    // and pg_class. Returns schema-qualified table names to match
    // QSqlDatabase::tables() output format for PostgreSQL.
    const QString query = QStringLiteral(
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
        "    AND (c.relname = :tableName OR (ns.nspname || '.' || c.relname) = :tableName) "
        "ORDER BY "
        "    ns.nspname, c.relname, a.attname, e.enumsortorder");

    QSqlQuery q(db);

    if (!q.prepare(query)) {
        qWarning("%s prepare failed: %ls", __func__, qUtf16Printable(q.lastError().text()));
        return result;
    }

    q.bindValue(QStringLiteral(":tableName"), tableName);

    if (!q.exec()) {
        qWarning("%s query failed: %ls", __func__, qUtf16Printable(q.lastError().text()));
        return result;
    }

    while (q.next()) {
        const QString table = q.value(0).toString();
        const QString column = q.value(1).toString();
        const QString value = q.value(2).toString();
        if (!table.isEmpty() && !column.isEmpty() && !value.isEmpty()) {
            result[column].append(value);
        }
    }

    return result;
}

TableEnums EnumHelper::getMySqlServerEnums(const QSqlDatabase &db, const QString &tableName)
{
    TableEnums result;

    // Query MySQL ENUM columns from INFORMATION_SCHEMA.COLUMNS.
    // COLUMN_TYPE for enum columns looks like: enum('val1','val2','val3')
    // We parse the values from this string.
    const QString query = QStringLiteral(
        "SELECT "
        "    TABLE_NAME, "
        "    COLUMN_NAME, "
        "    COLUMN_TYPE "
        "FROM "
        "    INFORMATION_SCHEMA.COLUMNS "
        "WHERE "
        "    TABLE_SCHEMA = DATABASE() "
        "    AND DATA_TYPE = 'enum' "
        "    AND TABLE_NAME = :tableName");

    QSqlQuery q(db);

    if (!q.prepare(query)) {
        qWarning("%s prepare failed: %ls", __func__, qUtf16Printable(q.lastError().text()));
        return result;
    }

    q.bindValue(QStringLiteral(":tableName"), tableName);

    if (!q.exec(query)) {
        qWarning("%s query exec failed: %ls", __func__, qUtf16Printable(q.lastError().text()));
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
            result[column] = values;
        }
    }

    return result;
}
