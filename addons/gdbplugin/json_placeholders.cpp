/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>

#include "json_placeholders.h"

namespace json
{
static const QRegularExpression rx_placeholder(QLatin1String(R"--(\$\{(#?[a-z]+(?:\.[a-z]+)*)(?:\|([a-z]+))?\})--"), QRegularExpression::CaseInsensitiveOption);
static const QRegularExpression rx_cast(QLatin1String(R"--(^\$\{(#?[a-z]+(?:\.[a-z]+)*)\|(int|bool|list)\}$)--"), QRegularExpression::CaseInsensitiveOption);

std::optional<QString> valueAsString(const QJsonValue &);

std::optional<QString> valueAsString(const QJsonArray &array, const bool quote = false)
{
    if (array.isEmpty())
        return QString();

    if (array.size() == 1)
        return valueAsString(array.first());

    QStringList parts;
    for (const auto &item : array) {
        const auto text = valueAsString(item);
        if (!text)
            return std::nullopt;
        if (quote)
            parts << QStringLiteral("\"%s\"").arg(text.value());
        else
            parts << *text;
    }
    return parts.join(QStringLiteral(" "));
}

std::optional<QString> valueAsString(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();
    if (value.isArray())
        return valueAsString(value.toArray());
    if (value.isBool())
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.isDouble())
        return QString::number(value.toDouble());

    return std::nullopt;
}

std::optional<QStringList> valueAsStringList(const QJsonValue &value)
{
    if (value.isArray()) {
        QStringList listValue;
        for (const auto &item : value.toArray()) {
            const auto text = valueAsString(item);
            if (!text)
                return std::nullopt;
            listValue << *text;
        }
        return listValue;
    } else {
        const auto text = valueAsString(value);
        if (text)
            return QProcess::splitCommand(*text);
    }

    return std::nullopt;
}

std::optional<bool> valueAsBool(const QJsonValue &value)
{
    if (value.isBool())
        return value.toBool();

    const auto text = valueAsString(value);
    if (text) {
        const auto cleanText = text->trimmed();
        if (cleanText == QStringLiteral("true")) {
            return true;
        } else if (cleanText == QStringLiteral("false")) {
            return false;
        }
    }
    return std::nullopt;
}

std::optional<int> valueAsInt(const QJsonValue &value)
{
    if (value.isDouble()) {
        return value.toInt();
    } else {
        const auto text = valueAsString(value);
        if (text) {
            bool ok = false;
            int intValue = text->trimmed().toInt(&ok, 10);
            if (ok) {
                return intValue;
            }
        }
    }
    return std::nullopt;
}

/**
 * @brief apply_filter
 * Apply a variable filter
 * @param text
 * @param filter
 * @return
 */
QString apply_filter(const QJsonValue &value, const QString &filter)
{
    const QString text = valueAsString(value).value_or(QString());
    if (filter == QStringLiteral("base")) {
        return QFileInfo(text).baseName();
    }
    if (filter == QStringLiteral("dir")) {
        return QFileInfo(text).dir().dirName();
    }
    return text;
}

/**
 * @brief cast_from_string
 * @param text
 * @param variables
 * @return QJsonValue if the replacement is successful
 */
std::optional<QJsonValue> cast_from_string(const QString &text, const VarMap &variables)
{
    const auto match = rx_cast.match(text);

    if (!match.hasMatch()) {
        return std::nullopt;
    }

    const auto &key = match.captured(1);
    if (!variables.contains(key)) {
        return std::nullopt;
    }

    const auto &filter = match.captured(2);
    const auto value = variables[key];

    if (filter == QStringLiteral("int")) {
        const auto &intValue = valueAsInt(value);
        if (intValue)
            return intValue.value();
    } else if (filter == QStringLiteral("bool")) {
        const auto &boolValue = valueAsBool(value);
        if (boolValue)
            return boolValue.value();
    } else if (filter == QStringLiteral("list")) {
        const auto &listValue = valueAsStringList(value);
        if (listValue)
            return QJsonArray::fromStringList(*listValue);
    }

    return std::nullopt;
}

QJsonValue resolve(const QString &text, const VarMap &variables)
{
    // try to cast to int/bool
    {
        auto casting = cast_from_string(text, variables);
        if (casting) {
            return *casting;
        }
    }

    QStringList parts;

    auto matches = rx_placeholder.globalMatch(text);

    int size = 0;
    while (matches.hasNext()) {
        const auto match = matches.next();
        const auto key = match.captured(1);
        if (!variables.contains(key)) {
            continue;
        }
        parts << text.mid(size, match.capturedStart(0)) << apply_filter(variables[key], match.captured(2));
        size = match.capturedEnd(0);
    }
    if (size == 0) {
        // not replaced
        return QJsonValue(text);
    }
    if (text.size() > size) {
        parts << text.mid(size);
    }

    return QJsonValue(parts.join(QStringLiteral("")));
}

QJsonObject resolve(const QJsonObject &map, const VarMap &variables)
{
    QJsonObject replaced;

    for (auto it = map.begin(); it != map.end(); ++it) {
        const auto &key = it.key();
        replaced[key] = resolve(*it, variables);
    }

    return replaced;
}

QJsonArray resolve(const QJsonArray &array, const VarMap &variables)
{
    QJsonArray replaced;

    for (const auto &value : array) {
        // if original == string and new is list, expand
        const auto newValue = resolve(value, variables);
        if (value.isString() && newValue.isArray()) {
            // concatenate
            for (const auto &item : newValue.toArray()) {
                replaced << item;
            }
        } else {
            replaced << newValue;
        }
    }

    return replaced;
}

QJsonValue resolve(const QJsonValue &value, const VarMap &variables)
{
    if (value.isObject())
        return resolve(value.toObject(), variables);

    if (value.isArray())
        return resolve(value.toArray(), variables);

    if (value.isString()) {
        return resolve(value.toString(), variables);
    }

    return value;
}

void findVariables(const QJsonObject &map, QSet<QString> &variables)
{
    if (map.isEmpty())
        return;
    for (const auto &value : map) {
        findVariables(value, variables);
    }
}

void findVariables(const QJsonValue &value, QSet<QString> &variables)
{
    if (value.isNull() || value.isUndefined())
        return;
    if (value.isObject()) {
        findVariables(value.toObject(), variables);
    } else if (value.isArray()) {
        findVariables(value.toArray(), variables);
    } else if (value.isString()) {
        findVariables(value.toString(), variables);
    }
}

void findVariables(const QJsonArray &array, QSet<QString> &variables)
{
    if (array.isEmpty())
        return;
    for (const auto &value : array) {
        findVariables(value, variables);
    }
}

void findVariables(const QString &text, QSet<QString> &variables)
{
    if (text.isNull() || text.isEmpty())
        return;
    auto matches = rx_placeholder.globalMatch(text);
    while (matches.hasNext()) {
        const auto match = matches.next();
        variables.insert(match.captured(1));
    }
}

}
