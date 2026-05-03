/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KConfigGroup>

#include <QMap>
#include <QString>
#include <QStringList>
#include <qtmetamacros.h>

#include "databaseconfigentryserializer.h"
#include "enumhelper.h"
#include "foreignkeyhelper.h"

/**
 * Utility class for serializing and deserializing database-related configuration.
 *
 * Handles persistent storage of:
 * - Foreign key relationships (DatabaseForeignKeys)
 * - Table to display column mappings (QMap<QString, QString>)
 *
 * Internally uses ConfigEntrySerializer<PartEnum> for entry-level
 * serialization. Each config entry type is defined by an enum whose values
 * name the parts and whose `Count` sentinel gives the field count at compile time.
 *
 * All methods accept a KConfigGroup parameter so the caller controls
 * the config lifecycle and avoids repeated KSharedConfig::openConfig() calls.
 */
class DatabaseConfigSerializerHelper
{
    Q_GADGET

public:
    /**
     * Reads foreign keys for a specific database from config.
     *
     * @param config The config group to read from.
     * @param databaseName The name of the database to read foreign keys for.
     * @return The deserialized foreign keys, or empty map if not found or on error.
     */
    static DatabaseForeignKeys readForeignKeys(const KConfigGroup &config, const QString &databaseName);

    /**
     * Writes foreign keys for a specific database to config.
     *
     * @param config The config group to write to.
     * @param databaseName The name of the database to write foreign keys for.
     * @param foreignKeys The foreign keys to serialize and store.
     */
    static void writeForeignKeys(KConfigGroup &config, const QString &databaseName, const DatabaseForeignKeys &foreignKeys);

    /**
     * Checks if foreign keys are stored in config for a specific database.
     *
     * @param config The config group to check.
     * @param databaseName The name of the database to check.
     * @return true if foreign keys exist in config, false otherwise.
     */
    static bool hasForeignKeys(const KConfigGroup &config, const QString &databaseName);

    /**
     * Removes cached foreign keys for a specific database from config.
     *
     * @param config The config group to modify.
     * @param databaseName The name of the database to remove foreign keys for.
     */
    static void removeForeignKeys(KConfigGroup &config, const QString &databaseName);

    /**
     * Reads table to display column mappings for a specific database from config.
     *
     * @param config The config group to read from.
     * @param databaseName The name of the database to read mappings for.
     * @return The deserialized mappings, or empty map if not found or on error.
     */
    static QMap<QString, QString> readTableToDisplayColumnMap(const KConfigGroup &config, const QString &databaseName);

    /**
     * Writes table to display column mappings for a specific database to config.
     *
     * @param config The config group to write to.
     * @param databaseName The name of the database to write mappings for.
     * @param displayColumns The mappings to serialize and store.
     */
    static void writeTableToDisplayColumnMap(KConfigGroup &config, const QString &databaseName, const QMap<QString, QString> &displayColumns);

    /**
     * Checks if table to display column mappings are stored in config for a specific database.
     *
     * @param config The config group to check.
     * @param databaseName The name of the database to check.
     * @return true if mappings exist in config, false otherwise.
     */
    static bool hasTableToDisplayColumnMap(const KConfigGroup &config, const QString &databaseName);

    /**
     * Reads enum column values for a specific database from config.
     *
     * @param config The config group to read from.
     * @param databaseName The name of the database to read enums for.
     * @return The deserialized enums, or empty map if not found or on error.
     */
    static DatabaseEnums readEnums(const KConfigGroup &config, const QString &databaseName);

    /**
     * Writes enum column values for a specific database to config.
     *
     * @param config The config group to write to.
     * @param databaseName The name of the database to write enums for.
     * @param enums The enum values to serialize and store.
     */
    static void writeEnums(KConfigGroup &config, const QString &databaseName, const DatabaseEnums &enums);

    /**
     * Checks if enum column values are stored in config for a specific database.
     *
     * @param config The config group to check.
     * @param databaseName The name of the database to check.
     * @return true if enums exist in config, false otherwise.
     */
    static bool hasEnums(const KConfigGroup &config, const QString &databaseName);

    /**
     * Removes cached enum column values for a specific database from config.
     *
     * @param config The config group to modify.
     * @param databaseName The name of the database to remove enums for.
     */
    static void removeEnums(KConfigGroup &config, const QString &databaseName);

private:
    /**
     * Enum identifying the parts of a serialized foreign-key entry.
     * Values are used as array indices; `Count` is a compile-time sentinel.
     */
    enum class ForeignKeyPart {
        TableWithForeignKey = 0,
        ColumnWithForeignKey = 1,
        ReferencedTable = 2,
        ReferencedColumn = 3,
        Count
    };
    Q_ENUM(ForeignKeyPart)

    /**
     * Enum identifying the parts of a serialized display-column entry.
     * Values are used as array indices; `Count` is a compile-time sentinel.
     */
    enum class DisplayColumnPart {
        Table = 0,
        DisplayColumn = 1,
        Count
    };
    Q_ENUM(DisplayColumnPart)

    /**
     * Enum identifying the parts of a serialized enum value entry.
     * Values are used as array indices; `Count` is a compile-time sentinel.
     */
    enum class EnumPart {
        Table = 0,
        Column = 1,
        Value = 2,
        Count
    };
    Q_ENUM(EnumPart)

    using ForeignKeyEntrySerializer = DatabaseConfigEntrySerializer<ForeignKeyPart>;
    using DisplayColumnEntrySerializer = DatabaseConfigEntrySerializer<DisplayColumnPart>;
    using EnumEntrySerializer = DatabaseConfigEntrySerializer<EnumPart>;
};
