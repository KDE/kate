/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KEYBOARDMACROSPLUGIN_MACRO_H
#define KEYBOARDMACROSPLUGIN_MACRO_H

#include <utility>

#include <QJsonArray>
#include <QJsonValue>
#include <QList>
#include <QRegularExpression>

#include "keycombination.h"

class Macro : public QList<KeyCombination>
{
public:
    explicit Macro()
        : QList<KeyCombination>(){};

    static const std::pair<const Macro, bool> fromJson(const QJsonValue &json)
    {
        if (json.type() != QJsonValue::Array) {
            std::pair(Macro(), false);
        }
        Macro macro;
        for (const auto &jsonKeyCombination : json.toArray()) {
            if (jsonKeyCombination.type() != QJsonValue::Array) {
                return std::pair(Macro(), false);
            }
            auto maybeKeyCombination = KeyCombination::fromJson(jsonKeyCombination.toArray());
            if (!maybeKeyCombination.second) {
                return std::pair(Macro(), false);
            }
            macro.append(maybeKeyCombination.first);
        }
        return std::pair(macro, true);
    };

    const QJsonArray toJson() const
    {
        QJsonArray json;
        Macro::ConstIterator it;
        for (it = this->constBegin(); it != this->constEnd(); ++it) {
            json.append(it->toJson());
        }
        return json;
    };

    const QString toString() const
    {
        QString str;
        for (const auto &kc : *this) {
            if (kc.isVisibleInput()) {
                str += kc.toString();
            } else {
                str += QStringLiteral(" ") + kc.toString() + QStringLiteral(" ");
            }
        }
        return str.trimmed().replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    };
};

#endif
