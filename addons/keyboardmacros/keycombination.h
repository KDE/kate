/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KEYBOARDMACROSPLUGIN_KEYCOMBINATION_H
#define KEYBOARDMACROSPLUGIN_KEYCOMBINATION_H

#include <QDebug>
#include <QJsonArray>
#include <QJsonValue>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <QtGlobal>

class KeyCombination
{
private:
    int key;
    Qt::KeyboardModifiers modifiers;
    QString text;

public:
    explicit KeyCombination(const QKeyEvent *keyEvent)
        : key(keyEvent->key())
        , modifiers(keyEvent->modifiers())
        , text(keyEvent->text()){};

    explicit KeyCombination(const QJsonArray &json)
    {
        Q_ASSERT(json.size() == 3);
        Q_ASSERT(json.at(0).type() == QJsonValue::Double);
        Q_ASSERT(json.at(1).type() == QJsonValue::Double);
        Q_ASSERT(json.at(2).type() == QJsonValue::String);
        key = json.at(0).toInt(0);
        modifiers = static_cast<Qt::KeyboardModifiers>(json.at(1).toInt(0));
        text = json.at(2).toString();
    };

    const QKeyEvent keyPress() const
    {
        return QKeyEvent(QEvent::KeyPress, key, modifiers, text);
    };

    const QKeyEvent keyRelease() const
    {
        return QKeyEvent(QEvent::KeyRelease, key, modifiers, text);
    };

    const QJsonArray toJson() const
    {
        QJsonArray json;
        json.append(QJsonValue(key));
        json.append(QJsonValue(static_cast<int>(modifiers)));
        json.append(QJsonValue(text));
        return json;
    };

    friend QDebug operator<<(QDebug dbg, const KeyCombination &kc)
    {
        return dbg << QKeySequence(kc.key | kc.modifiers).toString();
    };
};

#endif
