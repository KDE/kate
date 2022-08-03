/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KEYBOARDMACROSPLUGIN_MACRO_H
#define KEYBOARDMACROSPLUGIN_MACRO_H

#include <QDebug>
#include <QJsonArray>
#include <QJsonValue>
#include <QList>
#include <QtGlobal>

#include "keycombination.h"

class Macro : public QList<KeyCombination>
{
public:
    explicit Macro()
        : QList<KeyCombination>(){};

    explicit Macro(const QJsonValue &json)
    {
        Q_ASSERT(json.type() == QJsonValue::Array);
        QJsonArray::ConstIterator it;
        for (it = json.toArray().constBegin(); it != json.toArray().constEnd(); ++it) {
            Q_ASSERT(it->type() == QJsonValue::Array);
            this->append(KeyCombination(it->toArray()));
        }
    };

    QJsonArray toJson() const
    {
        QJsonArray json;
        Macro::ConstIterator it;
        for (it = this->constBegin(); it != this->constEnd(); ++it) {
            json.append(it->toJson());
        }
        return json;
    };
};

#endif
