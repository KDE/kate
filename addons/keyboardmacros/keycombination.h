/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KEYBOARDMACROSPLUGIN_KEYCOMBINATION_H
#define KEYBOARDMACROSPLUGIN_KEYCOMBINATION_H

#include <QApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonValue>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <QtGlobal>

class KeyCombination
{
private:
    int m_key;
    Qt::KeyboardModifiers m_modifiers;
    QString m_text;

public:
    explicit KeyCombination(const QKeyEvent *keyEvent)
        : m_key(keyEvent->key())
        , m_modifiers(keyEvent->modifiers())
        , m_text(keyEvent->text()){};

    explicit KeyCombination(const QJsonArray &json)
    {
        Q_ASSERT(json.size() == 3);
        Q_ASSERT(json.at(0).type() == QJsonValue::Double);
        Q_ASSERT(json.at(1).type() == QJsonValue::Double);
        Q_ASSERT(json.at(2).type() == QJsonValue::String);
        m_key = json.at(0).toInt(0);
        m_modifiers = static_cast<Qt::KeyboardModifiers>(json.at(1).toInt(0));
        m_text = json.at(2).toString();
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
        return m_text.size() == 1 && (m_modifiers == Qt::NoModifier || m_modifiers == Qt::ShiftModifier) && m_text.at(0).isPrint();
    }

    friend QDebug operator<<(QDebug dbg, const KeyCombination &kc)
    {
        return dbg << kc.toString();
    };
};

#endif
