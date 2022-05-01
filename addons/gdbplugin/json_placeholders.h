/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef JSON_PLACEHOLDERS_H
#define JSON_PLACEHOLDERS_H

#include <QHash>
#include <QSet>
#include <QString>
#include <optional>
#include <utility>

class QJsonObject;
class QJsonArray;
class QJsonValue;

/**
 * @brief Utility for replacing variable placeholders in JSON documents
 *
 * Format:
 *
 *  ${variable} -> key="variable"
 *  ${long.variable} -> key="long.variable"
 *  ${#variable} -> key="#variable"
 *  ${#long.variable} -> key="#long.variable"
 *
 * A variable can be transformed by using a filter:
 *
 *  ${variable|base} -> file base name
 *  ${variable|dir} -> parent dir
 *
 *  ${variable|int} -> transform to an integer value
 *  ${variable|bool} -> transform to a boolean value
 *  ${variable|list} -> transform or keep as a stringlist
 *
 * "int" and "bool" filters are only applied when the string contains only the placeholder
 */
namespace json
{
typedef QHash<QString, QJsonValue> VarMap;

/**
 * @brief findVariables find variable templates and store in `variables`
 * @param map
 * @param variables
 */
void findVariables(const QJsonObject &map, QSet<QString> &variables);
void findVariables(const QJsonValue &value, QSet<QString> &variables);
void findVariables(const QJsonArray &array, QSet<QString> &variables);
void findVariables(const QString &text, QSet<QString> &variables);

/**
 * @brief resolve replace variable templates with the values in `variables`
 * @param text
 * @param variables
 * @return
 */
QJsonValue resolve(const QString &text, const VarMap &variables);
QJsonObject resolve(const QJsonObject &map, const VarMap &variables);
QJsonArray resolve(const QJsonArray &array, const VarMap &variables);
QJsonValue resolve(const QJsonValue &value, const VarMap &variables);

}

#endif // JSON_PLACEHOLDERS_H
