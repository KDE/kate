/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaType>
#include <QString>
#include <optional>

#include "records.h"

namespace gdbmi
{

template<typename T>
struct Item {
    // head position
    int position;
    // parsed value
    std::optional<T> value;
    // error message
    std::optional<QString> error;

    bool isEmpty() const
    {
        return !value.has_value();
    }

    bool hasError() const
    {
        return isEmpty() && error.has_value();
    }
};

/**
 * make an empty item (no value)
 */
template<typename T>
Item<T> make_empty_item(int position)
{
    return Item<T>{position, std::nullopt, std::nullopt};
}

/**
 * make an empty item with an error message
 */
template<typename T>
Item<T> make_error_item(int position, const QString &error)
{
    return Item<T>{position, std::nullopt, error};
}

/**
 * make an item with value
 */
template<typename T>
Item<T> make_item(int position, T &&value)
{
    return Item<T>{position, std::forward<T>(value), std::nullopt};
}

/**
 * convert an empty item from type Told to type Tnew
 */
template<typename Tnew, typename Told>
Item<Tnew> relay_item(const Item<Told> &item, const int pos = -1)
{
    return Item<Tnew>{pos >= 0 ? pos : item.position, std::nullopt, item.error};
}

struct Result {
    QString name;
    QJsonValue value;
};

/**
 * unescape a string
 */
QString unescapeString(const QByteArray &escapedString, QJsonParseError *error);
/**
 * escape quotes in a string
 */
QString quotedString(const QString &message);

int advanceBlanks(const QByteArray &buffer, int position);
int advanceNewlines(const QByteArray &buffer, int position);
int indexOfNewline(const QByteArray &buffer, const int offset);

/**
 * extract a prompt separator
 */
Item<int> tryPrompt(const QByteArray &buffer, const int start);
/**
 * extract sequence token
 *
 * token → any sequence of digits.
 */
Item<int> tryToken(const QByteArray &buffer, const int start);
/**
 * extract c-string
 */
Item<QString> tryString(const QByteArray &buffer, int start);
/**
 * extract classname
 *
 * result-record → [ token ] "^" result-class ( "," result )* nl
 */
Item<QString> tryClassName(const QByteArray &buffer, const int start);
/**
 * extract variable
 *
 * result → variable "=" value
 */
Item<QString> tryVariable(const QByteArray &message, const int start, const char sep = '=');
/**
 * extract stream output
 *
 * console-stream-output → "~" c-string nl
 * target-stream-output → "@" c-string nl
 * log-stream-output → "&" c-string nl
 */
Item<StreamOutput> tryStreamOutput(const char prefix, const QByteArray &buffer, int position);

/**
 * extract result
 *
 * result → variable "=" value
 */
Item<Result> tryResult(const QByteArray &message, const int start);
/**
 * result ( "," result )*
 *
 * @return {key1=value1, ..., keyN=valueN}
 */
Item<QJsonObject> tryResults(const QByteArray &message, const int start);
/**
 * result ( "," result )*
 *
 * @return [{key1=value1}, ..., {keyN=valueN}]
 */
Item<QJsonArray> tryResultList(const QByteArray &message, const int start);
/**
 * value → const | tuple | list
 */
Item<QJsonValue> tryValue(const QByteArray &message, const int start);
/**
 * value ( "," value )*
 */
Item<QJsonArray> tryValueList(const QByteArray &message, const int start);
/**
 * tuple → "{}" | "{" result ( "," result )* "}"
 */
Item<QJsonObject> tryTuple(const QByteArray &message, const int start);
/**
 * tuple → "[]" | "[" result ( "," result )* "]"
 */
Item<QJsonArray> tryTupleList(const QByteArray &message, const int start);
/**
 * list → "[]" | "[" value ( "," value )* "]" | "[" result ( "," result )* "]"
 */
Item<QJsonValue> tryList(const QByteArray &message, const int start);
/**
 * exec-async-output → [ token ] "*" async-output nl
 * status-async-output → [ token ] "+" async-output nl
 * notify-async-output → [ token ] "=" async-output nl
 * async-output → async-class ( "," result )*
 *
 * result-record → [ token ] "^" result-class ( "," result )* nl
 */
Item<Record> tryRecord(const char prefix, const QByteArray &message, const int start, int token = -1);
}

Q_DECLARE_METATYPE(gdbmi::StreamOutput)
Q_DECLARE_METATYPE(gdbmi::Record)
