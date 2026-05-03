/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QLatin1Char>
#include <QString>
#include <QStringList>

#include <array>
#include <optional>
#include <type_traits>

/**
 * Template for serializing/deserializing config entries defined by an enum of parts.
 *
 * Each config entry is a separator-delimited string where each field corresponds
 * to an enum value. The enum must have a `Count` sentinel as its last value.
 *
 * Usage:
 * @code
 *   enum MyPart { Name = 0, Value = 1, Count };
 *   using MySerializer = ConfigEntrySerializer<MyPart>;
 *
 *   QString s = MySerializer::serialize({{"foo", "bar"}});
 *   auto result = MySerializer::deserialize(s); // std::optional<std::array<QString, 2>>
 * @endcode
 *
 * @tparam PartEnum An enum whose values are sequential from 0, with `Count` as the last entry.
 */
/**
 * Converts a scoped-enum value to its underlying int index.
 *
 * Intended for indexing into the std::array produced by DatabaseConfigEntrySerializer::deserialize().
 *
 * Example:
 * @code
 *   auto parts = MySerializer::deserialize(entry);
 *   QString name = (*parts)[toIndex(MyPart::Name)];
 * @endcode
 */
template<typename Enum>
    requires std::is_enum_v<Enum>
constexpr int toInt(Enum e)
{
    return static_cast<int>(e);
}

template<typename PartEnum>
    requires std::is_enum_v<PartEnum>
class DatabaseConfigEntrySerializer
{
public:
    /**
     * A symbol guaranteed to not appear in schema/table/column names
     */
    static constexpr QLatin1Char Separator = QLatin1Char('\x01');

    /**
     * The number of parts in a config entry, derived at compile time from the enum.
     * Requires PartEnum to have a `Count` member equal to the number of valid parts.
     */
    static constexpr int PartCount = static_cast<int>(PartEnum::Count);

    static_assert(PartCount > 0, "PartEnum::Count must be greater than 0");

    /**
     * Serializes an array of parts into a separator-delimited string.
     */
    static QString serialize(const std::array<QString, PartCount> &parts)
    {
        return QStringList(parts.cbegin(), parts.cend()).join(QString(Separator));
    }

    /**
     * Deserializes a separator-delimited string into an array of parts.
     *
     * @return The deserialized parts, or std::nullopt if the entry is malformed
     *         (wrong number of fields or any field is empty).
     */
    static std::optional<std::array<QString, PartCount>> deserialize(const QString &entry)
    {
        const QStringList parts = entry.split(Separator);
        if (parts.size() != PartCount) {
            return std::nullopt;
        }

        std::array<QString, PartCount> result;
        for (int i = 0; i < PartCount; ++i) {
            if (parts[i].isEmpty()) {
                return std::nullopt;
            }
            result[i] = parts[i];
        }
        return result;
    }

    /**
     * Compile-time check whether an index is a valid part position.
     */
    static constexpr bool isValidPartIndex(int index)
    {
        return index >= 0 && index < PartCount;
    }
};
