/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#include "databaseconfigserializinghelper.h"
#include "foreignkeyhelper.h"

DatabaseForeignKeys DatabaseConfigSerializerHelper::readForeignKeys(const KConfigGroup &config, const QString &databaseName)
{
    DatabaseForeignKeys result;

    if (!config.hasKey(databaseName)) {
        return result;
    }

    const QStringList serializedKeys = config.readEntry(databaseName, QStringList());

    for (const QString &entry : serializedKeys) {
        auto parts = ForeignKeyEntrySerializer::deserialize(entry);
        if (!parts) {
            continue;
        }

        const auto &p = *parts;
        const auto tableWithFk = p[toInt(ForeignKeyPart::TableWithForeignKey)];
        const auto columnWithFk = p[toInt(ForeignKeyPart::ColumnWithForeignKey)];
        const auto referencedTable = p[toInt(ForeignKeyPart::ReferencedTable)];
        const auto referencedColumn = p[toInt(ForeignKeyPart::ReferencedColumn)];
        result[tableWithFk][columnWithFk] = qMakePair(referencedTable, referencedColumn);
    }

    return result;
}

void DatabaseConfigSerializerHelper::writeForeignKeys(KConfigGroup &config, const QString &databaseName, const DatabaseForeignKeys &foreignKeys)
{
    QStringList serializedKeys;
    serializedKeys.reserve(foreignKeys.size());

    for (auto tableIt = foreignKeys.constBegin(); tableIt != foreignKeys.constEnd(); ++tableIt) {
        const QString &table = tableIt.key();
        const ColumnForeignKeys &columnFks = tableIt.value();

        for (auto colIt = columnFks.constBegin(); colIt != columnFks.constEnd(); ++colIt) {
            const QString &column = colIt.key();
            const ColumnReference &ref = colIt.value();
            serializedKeys.append(ForeignKeyEntrySerializer::serialize({table, column, ref.first, ref.second}));
        }
    }

    config.writeEntry(databaseName, serializedKeys);
    config.sync();
}

bool DatabaseConfigSerializerHelper::hasForeignKeys(const KConfigGroup &config, const QString &databaseName)
{
    return config.hasKey(databaseName);
}

void DatabaseConfigSerializerHelper::removeForeignKeys(KConfigGroup &config, const QString &databaseName)
{
    config.deleteEntry(databaseName);
    config.sync();
}

QMap<QString, QString> DatabaseConfigSerializerHelper::readTableToDisplayColumnMap(const KConfigGroup &config, const QString &databaseName)
{
    QMap<QString, QString> result;

    if (!config.hasKey(databaseName)) {
        return result;
    }

    const QStringList serializedMappings = config.readEntry(databaseName, QStringList());

    for (const QString &entry : serializedMappings) {
        auto parts = DisplayColumnEntrySerializer::deserialize(entry);
        if (parts) {
            const auto &p = *parts;
            const auto table = p[toInt(DisplayColumnPart::Table)];
            const auto displayColumn = p[toInt(DisplayColumnPart::DisplayColumn)];
            result[table] = displayColumn;
        }
    }

    return result;
}

void DatabaseConfigSerializerHelper::writeTableToDisplayColumnMap(KConfigGroup &config,
                                                                  const QString &databaseName,
                                                                  const QMap<QString, QString> &displayColumns)
{
    QStringList serializedMappings;
    serializedMappings.reserve(displayColumns.size());

    for (auto it = displayColumns.constBegin(); it != displayColumns.constEnd(); ++it) {
        serializedMappings.append(DisplayColumnEntrySerializer::serialize({it.key(), it.value()}));
    }

    config.writeEntry(databaseName, serializedMappings);
    config.sync();
}

bool DatabaseConfigSerializerHelper::hasTableToDisplayColumnMap(const KConfigGroup &config, const QString &databaseName)
{
    return config.hasKey(databaseName);
}

DatabaseEnums DatabaseConfigSerializerHelper::readEnums(const KConfigGroup &config, const QString &databaseName)
{
    DatabaseEnums result;

    if (!config.hasKey(databaseName)) {
        return result;
    }

    const QStringList serializedEnums = config.readEntry(databaseName, QStringList());

    for (const QString &entry : serializedEnums) {
        auto parts = EnumEntrySerializer::deserialize(entry);
        if (!parts) {
            continue;
        }

        const auto &p = *parts;
        const auto table = p[toInt(EnumPart::Table)];
        const auto column = p[toInt(EnumPart::Column)];
        const auto value = p[toInt(EnumPart::Value)];
        result[table][column].append(value);
    }

    return result;
}

void DatabaseConfigSerializerHelper::writeEnums(KConfigGroup &config, const QString &databaseName, const DatabaseEnums &enums)
{
    QStringList serializedEnums;

    for (auto tableIt = enums.constBegin(); tableIt != enums.constEnd(); ++tableIt) {
        const QString &table = tableIt.key();
        const TableEnums &tableEnums = tableIt.value();

        for (auto colIt = tableEnums.constBegin(); colIt != tableEnums.constEnd(); ++colIt) {
            const QString &column = colIt.key();
            const ColumnEnums &values = colIt.value();

            for (const QString &value : values) {
                serializedEnums.append(EnumEntrySerializer::serialize({table, column, value}));
            }
        }
    }

    config.writeEntry(databaseName, serializedEnums);
    config.sync();
}

bool DatabaseConfigSerializerHelper::hasEnums(const KConfigGroup &config, const QString &databaseName)
{
    return config.hasKey(databaseName);
}

void DatabaseConfigSerializerHelper::removeEnums(KConfigGroup &config, const QString &databaseName)
{
    config.deleteEntry(databaseName);
    config.sync();
}
