/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tokens.h"
#include <QString>

namespace gdbmi
{

/**
 * @brief eatSpace
 * @param message
 * @param position
 * @return new position
 */
int advanceBlanks(const QByteArray &buffer, int position)
{
    const int size = buffer.size();
    while (position < size) {
        const char head = buffer.at(position);
        if ((head != ' ') && (head != '\t')) {
            break;
        }
        ++position;
    }
    return position;
}

int advanceNewlines(const QByteArray &buffer, int position)
{
    if (position < 0) {
        return position;
    }

    const int size = buffer.size();
    while (position < size) {
        const char head = buffer.at(position);
        if ((head != ' ') && (head != '\t') && (head != '\n') && (head != '\r')) {
            break;
        }
        ++position;
    }
    return position;
}

/**
 * index of char or newline
 */
static int findInLine(const QByteArray &buffer, char item, int position)
{
    if (position < 0) {
        return -1;
    }
    const int size = buffer.size();
    while (position < size) {
        const char head = buffer.at(position);
        if ((head == item) || (head == '\r') || (head == '\n')) {
            break;
        }
        ++position;
    }

    if (position < size) {
        return position;
    }
    return -1;
}

int indexOfNewline(const QByteArray &buffer, const int offset)
{
    // \r\n
    const int r_idx = buffer.indexOf('\r', offset);
    if ((r_idx >= 0) && ((r_idx + 1) < buffer.size()) && (buffer.at(r_idx + 1) == '\n')) {
        return r_idx;
    }

    const int n_idx = buffer.indexOf('\n', offset);
    if (n_idx >= 0) {
        // \n
        return n_idx;
    } else {
        // \r
        return r_idx;
    }
}

QString unescapeString(const QByteArray &escapedString, QJsonParseError *error)
{
    // hack: use json to unescape c-string
    // maybe change to std::quoted
    const auto doc = QJsonDocument::fromJson(QByteArrayLiteral("[\"") + escapedString + QByteArrayLiteral("\"]"), error);
    if ((error != nullptr) && (error->error != QJsonParseError::NoError)) {
        return QString();
    }
    return doc.array()[0].toString();
}

QString quotedString(const QString &message)
{
    static const QRegularExpression rx(QStringLiteral(R"--(((?<!\\)\"))--"));
    return QString(message).replace(rx, QStringLiteral(R"--(\")--"));
}

Item<int> tryPrompt(const QByteArray &buffer, const int start)
{
    const int size = buffer.size() - start;

    const bool is_prompt = (size >= 5) && (buffer.at(start) == '(') && (buffer.at(start + 1) == 'g') && (buffer.at(start + 2) == 'd')
        && (buffer.at(start + 3) == 'b') && (buffer.at(start + 4) == ')');

    if (!is_prompt) {
        return make_error_item<int>(start, QStringLiteral("unexpected prompt format"));
    }

    return make_empty_item<int>(start + 5);
}

Item<QString> tryString(const QByteArray &buffer, int start)
{
    int position = advanceBlanks(buffer, start);

    const int size = buffer.size();

    if (position >= size) {
        return make_empty_item<QString>(start);
    }

    char head = buffer.at(position);
    if (head != '"') {
        return make_error_item<QString>(start, QStringLiteral("unexpected string delimiter"));
    }

    const int strStart = ++position;

    bool mustUnescape = false;
    bool escaped = false;

    while (position < size) {
        head = buffer.at(position);
        if ((head == '"') && !escaped)
            break;
        escaped = !escaped && (head == '\\');
        mustUnescape = mustUnescape || escaped;
        ++position;
    }

    if (position >= size) {
        return make_error_item<QString>(start, QStringLiteral("unexpected end of buffer"));
    }

    const auto &roi = buffer.mid(strStart, position - start - 1);
    if (!mustUnescape) {
        return make_item(position + 1, QString::fromLocal8Bit(roi));
    }
    // use json to unescape c-string
    QJsonParseError error;
    QString finalString = unescapeString(roi, &error);
    if (error.error != QJsonParseError::NoError) {
        return make_error_item<QString>(start, error.errorString());
    }

    return make_item(position + 1, std::move(finalString));
}

/**
 * @brief findToken
 * @param message
 * @param start
 * @return [position, value]
 */
Item<int> tryToken(const QByteArray &buffer, const int start)
{
    int position = start;
    const int size = buffer.size();
    while (position < size) {
        const char head = buffer.at(position);
        if ((head < '0') || (head > '9')) {
            break;
        }
        ++position;
    }

    if (position > start) {
        return make_item(position, buffer.mid(start, position - start).toInt());
    }

    return make_empty_item<int>(position);
}

Item<QString> tryClassName(const QByteArray &buffer, const int start)
{
    int position = advanceBlanks(buffer, start);
    if (position >= buffer.size()) {
        return make_error_item<QString>(start, QStringLiteral("unexpected end on line"));
    }
    int end = findInLine(buffer, ',', position);
    if (end < 0) {
        end = indexOfNewline(buffer, position);
        if (end < 0) {
            return make_item(buffer.size(), QString::fromLocal8Bit(buffer.mid(position)).trimmed());
        } else {
            return make_item(end, QString::fromLocal8Bit(buffer.mid(position, end - position)).trimmed());
        }
    } else {
        return make_item(end, QString::fromLocal8Bit(buffer.mid(position, end - position)).trimmed());
    }
}

Item<QString> tryVariable(const QByteArray &message, const int start, const char sep)
{
    int position = advanceBlanks(message, start);
    if (position >= message.size()) {
        return make_error_item<QString>(start, QStringLiteral("unexpected end of variable"));
    }
    int end = findInLine(message, sep, position);
    if (end < 0) {
        return make_error_item<QString>(start, QStringLiteral("result name separator '=' not found"));
    }

    return make_item(end + 1, QString::fromLocal8Bit(message.mid(position, end - position)).trimmed());
}

Item<StreamOutput> tryStreamOutput(const char prefix, const QByteArray &buffer, int start)
{
    /*
     * console-stream-output → "~" c-string nl
     * target-stream-output → "@" c-string nl
     * log-stream-output → "&" c-string nl
     */
    Item<StreamOutput> out;

    int position = start + 1;

    QString message;
    const auto strTok = tryString(buffer, position);
    if (!strTok.isEmpty()) {
        message = strTok.value.value();
        out.position = advanceNewlines(buffer, strTok.position);
    } else {
        int idx = indexOfNewline(buffer, position);
        if (idx < 0) {
            message = QString::fromLocal8Bit(buffer.mid(position));
            out.position = buffer.size();
        } else {
            message = QString::fromLocal8Bit(buffer.mid(position, idx - position));
            out.position = advanceNewlines(buffer, idx);
        }
    }

    StreamOutput payload;

    payload.message = message;
    if (prefix == '~') {
        payload.channel = StreamOutput::Console;
    } else if (prefix == '@') {
        payload.channel = StreamOutput::Output;
    } else if (prefix == '&') {
        payload.channel = StreamOutput::Log;
    } else {
        make_error_item<StreamOutput>(start, QStringLiteral("unknown streamoutput prefix"));
    }
    out.value = std::move(payload);

    return out;
}

Item<Result> tryResult(const QByteArray &message, const int start)
{
    /*
     * result → variable "=" value
     */
    int position = advanceBlanks(message, start);
    if (position >= message.size()) {
        return make_empty_item<Result>(position);
    }

    // variable
    const auto tokVariable = tryVariable(message, position);
    if (tokVariable.isEmpty()) {
        return relay_item<Result>(tokVariable);
    }

    Result out;
    out.name = tokVariable.value.value();

    position = advanceBlanks(message, tokVariable.position);
    if (position >= message.size()) {
        return make_error_item<Result>(start, QStringLiteral("unexpected end of result"));
    }

    const auto tokValue = tryValue(message, position);
    if (tokValue.isEmpty()) {
        return relay_item<Result>(tokValue, start);
    }

    out.value = tokValue.value.value();

    return make_item(tokValue.position, std::move(out));
}

Item<QJsonObject> tryResults(const QByteArray &message, const int start)
{
    QJsonObject out;
    const int size = message.size();

    int position = start;
    do {
        if (position > start) {
            // skip ','
            ++position;
        }
        const auto tokResult = tryResult(message, position);
        if (tokResult.isEmpty()) {
            return relay_item<QJsonObject>(tokResult);
        }
        out[tokResult.value->name] = tokResult.value->value;
        position = advanceBlanks(message, tokResult.position);
    } while ((position < size) && (message.at(position) == ','));

    return make_item(position, std::move(out));
}

Item<QJsonArray> tryResultList(const QByteArray &message, const int start)
{
    QJsonArray out;
    const int size = message.size();

    int position = start;
    do {
        if (position > start) {
            // skip ','
            ++position;
        }
        const auto tokResult = tryResult(message, position);
        if (tokResult.isEmpty()) {
            return relay_item<QJsonArray>(tokResult);
        }
        out.push_back(QJsonObject{{tokResult.value->name, tokResult.value->value}});
        position = advanceBlanks(message, tokResult.position);
    } while ((position < size) && (message.at(position) == ','));

    return make_item(position, std::move(out));
}

Item<QJsonArray> tryValueList(const QByteArray &message, const int start)
{
    QJsonArray out;

    int position = start;
    do {
        if (position > start) {
            // skip ','
            ++position;
        }
        const auto tok = tryValue(message, position);
        if (tok.isEmpty()) {
            return relay_item<QJsonArray>(tok);
        }
        out << tok.value.value();
        position = advanceBlanks(message, tok.position);
    } while ((position < message.size()) && (message.at(position) == ','));

    return make_item(position, std::move(out));
}

Item<QJsonValue> tryValue(const QByteArray &message, const int start)
{
    /*
     * value → const | tuple | list
     */
    int position = advanceBlanks(message, start);
    if (position >= message.size()) {
        return make_error_item<QJsonValue>(start, QStringLiteral("unexpected end of value"));
    }

    QJsonValue out;

    const char head = message.at(position);
    switch (head) {
    case '"': {
        const auto tokString = tryString(message, position);
        if (tokString.isEmpty()) {
            return relay_item<QJsonValue>(tokString);
        }
        out = tokString.value.value();
        position = tokString.position;
    } break;
    case '{':
        // tuple
        {
            const auto tokTuple = tryTuple(message, position);
            if (tokTuple.isEmpty()) {
                return relay_item<QJsonValue>(tokTuple);
            }
            out = tokTuple.value.value();
            position = tokTuple.position;
        }
        break;
    case '[':
        // list
        {
            const auto tokList = tryList(message, position);
            if (tokList.isEmpty()) {
                return relay_item<QJsonValue>(tokList);
            }
            out = tokList.value.value();
            position = tokList.position;
        }
        break;
    default:
        return make_error_item<QJsonValue>(start, QStringLiteral("unexpected character"));
    }

    return make_item(position, std::move(out));
}

Item<QJsonObject> tryTuple(const QByteArray &message, const int start)
{
    /*
     * tuple → "{}" | "{" result ( "," result )* "}"
     */
    // advance { or [
    int position = advanceBlanks(message, start + 1);
    if (position >= message.size()) {
        return make_error_item<QJsonObject>(start, QStringLiteral("unexpected end of tuple"));
    }

    QJsonObject out;

    // empty tuple ?
    if (message.at(position) != '}') {
        const auto tokResults = tryResults(message, position);
        if (!tokResults.isEmpty()) {
            position = advanceBlanks(message, tokResults.position);
        }
        if ((position >= message.size()) || (message.at(position) != '}')) {
            return make_error_item<QJsonObject>(start, QStringLiteral("unexpected end of tuple"));
        }
        if (tokResults.value) {
            out = tokResults.value.value();
        }
    }
    return make_item(position + 1, std::move(out));
}

Item<QJsonArray> tryTupleList(const QByteArray &message, const int start)
{
    /*
     * list → "[]" | "[" result ( "," result )* "]"
     */
    // advance { or [
    int position = advanceBlanks(message, start + 1);
    if (position >= message.size()) {
        return make_error_item<QJsonArray>(start, QStringLiteral("unexpected end of list"));
    }

    QJsonArray out;

    // empty tuple ?
    if (message.at(position) != ']') {
        const auto tokResults = tryResultList(message, position);
        if (!tokResults.isEmpty()) {
            position = advanceBlanks(message, tokResults.position);
        }
        if ((position >= message.size()) || (message.at(position) != ']')) {
            return make_error_item<QJsonArray>(start, QStringLiteral("unexpected end of list"));
        }
        if (tokResults.value) {
            out = tokResults.value.value();
        }
    }
    return make_item(position + 1, std::move(out));
}

Item<QJsonValue> tryList(const QByteArray &message, const int start)
{
    /*
     * list → "[]" | "[" value ( "," value )* "]" | "[" result ( "," result )* "]"
     */
    int position = advanceBlanks(message, start);
    if (position >= message.size()) {
        return make_error_item<QJsonValue>(start, QStringLiteral("unexpected end of list"));
    }

    // try values
    const auto tokTuple = tryTupleList(message, position);
    if (!tokTuple.isEmpty())
        return make_item(tokTuple.position, QJsonValue(tokTuple.value.value()));

    // skip [
    const auto tokValues = tryValueList(message, position + 1);
    if (!tokValues.isEmpty()) {
        position = advanceBlanks(message, tokValues.position);
    }

    if ((position >= message.size()) || (message.at(position) != ']')) {
        return make_error_item<QJsonValue>(start, QStringLiteral("unexpected end of list"));
    }

    QJsonValue out;
    if (tokValues.value) {
        out = tokValues.value.value();
    }
    return make_item(position + 1, std::move(out));
}

Item<Record> tryRecord(const char prefix, const QByteArray &message, const int start, int token)
{
    /*
     * exec-async-output → [ token ] "*" async-output nl
     * status-async-output → [ token ] "+" async-output nl
     * notify-async-output → [ token ] "=" async-output nl
     * async-output → async-class ( "," result )*
     *
     * result-record → [ token ] "^" result-class ( "," result )* nl
     */

    const auto className = tryClassName(message, start + 1);
    if (className.hasError()) {
        return relay_item<Record>(className);
    } else if (className.isEmpty()) {
        return make_error_item<Record>(className.position, QStringLiteral("class name not found"));
    }

    Record payload;
    payload.resultClass = className.value.value();

    if (token >= 0) {
        payload.token = token;
    }
    if (prefix == '*') {
        payload.category = Record::Exec;
    } else if (prefix == '+') {
        payload.category = Record::Status;
    } else if (prefix == '=') {
        payload.category = Record::Notify;
    } else if (prefix == '^') {
        payload.category = Record::Result;
    }

    // parse results
    int position = advanceBlanks(message, className.position);
    if ((position >= message.size()) || (message.at(position) != ',')) {
        return make_item(position, std::move(payload));
    }

    const auto tokResults = tryResults(message, ++position);
    if (tokResults.isEmpty()) {
        return relay_item<Record>(tokResults);
    }

    payload.value = tokResults.value.value();

    position = advanceNewlines(message, tokResults.position);

    return make_item(position, std::move(payload));
}

} // namespace gdbmi
