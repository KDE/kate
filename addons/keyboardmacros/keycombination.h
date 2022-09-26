/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KEYBOARDMACROSPLUGIN_KEYCOMBINATION_H
#define KEYBOARDMACROSPLUGIN_KEYCOMBINATION_H

#include <utility>

#include <QDebug>
#include <QJsonArray>
#include <QJsonValue>
#include <QKeyEvent>
#include <QKeySequence>
#include <QString>

class KeyCombination
{
private:
    int m_key = -1;
    Qt::KeyboardModifiers m_modifiers;
    QString m_text;

public:
    KeyCombination(){};

    KeyCombination(const int key, const Qt::KeyboardModifiers modifiers, const QString &text)
        : m_key(key)
        , m_modifiers(modifiers)
        , m_text(text){};

    explicit KeyCombination(const QKeyEvent *keyEvent)
        : m_key(keyEvent->key())
        , m_modifiers(keyEvent->modifiers())
        , m_text(keyEvent->text()){};

    static const std::pair<const KeyCombination, bool> fromJson(const QJsonArray &json)
    {
        if (json.size() != 3 || json[0].type() != QJsonValue::Double || json[1].type() != QJsonValue::Double || json[2].type() != QJsonValue::String) {
            return std::pair(KeyCombination(), false);
        }
        return std::pair(KeyCombination(json[0].toInt(0), static_cast<Qt::KeyboardModifiers>(json[1].toInt(0)), json[2].toString()), true);
    };

    const QKeyEvent keyPress() const
    {
        return QKeyEvent(QEvent::KeyPress, m_key, m_modifiers, m_text);
    };

    const QKeyEvent keyRelease() const
    {
        return QKeyEvent(QEvent::KeyRelease, m_key, m_modifiers, m_text);
    };

    const QJsonArray toJson() const
    {
        QJsonArray json;
        json.append(QJsonValue(m_key));
        json.append(QJsonValue(static_cast<int>(m_modifiers)));
        json.append(QJsonValue(m_text));
        return json;
    };

    const QString toString() const
    {
        if (isVisibleInput()) {
            return m_text;
        }
        return QKeySequence(m_key | m_modifiers).toString();
    };

    bool isVisibleInput() const
    {
        return m_text.size() == 1 && (m_modifiers == Qt::NoModifier || m_modifiers == Qt::ShiftModifier) && m_text[0].isPrint();
    };

    friend QDebug operator<<(QDebug dbg, const KeyCombination &kc)
    {
        return dbg << kc.toString();
    };
};

#endif
