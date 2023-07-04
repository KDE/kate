/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "parser.h"
#include "tokens.h"
#include <QJsonArray>
#include <QRegularExpression>

namespace gdbmi
{

int GdbmiParser::parseRecord(const QByteArray &message, int position)
{
    /*
     * out-of-band-record → async-record | stream-record
     */
    position = advanceNewlines(message, position);

    if (position >= message.size()) {
        return position;
    }

    // token
    const auto token = tryToken(message, position);
    if (!token.isEmpty()) {
        position = token.position;
    }

    if (position >= message.size()) {
        Q_EMIT parserError(QStringLiteral("unexpected end of line"));
        return -1;
    }

    const auto prefix = message.at(position);

    // async-output * + =
    switch (prefix) {
    case '~':
    case '@':
    case '&': {
        const auto tok = tryStreamOutput(prefix, message, position);
        if (!tok.isEmpty()) {
            Q_EMIT outputProduced(tok.value.value());
            position = tok.position;
        } else if (tok.hasError()) {
            Q_EMIT parserError(tok.error.value());
            position = -1;
        } else {
            position = tok.position;
        }
    } break;
    // stream-output ~ @ & ^
    case '*':
    case '+':
    case '=':
    case '^': {
        const auto tok = tryRecord(prefix, message, position, token.value.value_or(-1));
        if (!tok.isEmpty()) {
            Q_EMIT recordProduced(tok.value.value());
            position = tok.position;
        } else if (tok.hasError()) {
            Q_EMIT parserError(tok.error.value());
            position = -1;
        } else {
            position = tok.position;
        }
    } break;
    case '(': {
        const auto tok = tryPrompt(message, position);
        if (!tok.hasError()) {
            Q_EMIT recordProduced(Record{Record::Prompt, QString(), {}, std::nullopt});
            position = tok.position;
        } else {
            Q_EMIT parserError(tok.error.value());
            position = -1;
        }
        break;
    }
    default:
        Q_EMIT parserError(QStringLiteral("unexpected GDB/MI record class: %1").arg(prefix));
        return -1;
    }

    return advanceNewlines(message, position);
}

GdbmiParser::GdbmiParser(QObject *parent)
    : QObject(parent)
{
}

GdbmiParser::ParserHead GdbmiParser::parseResponse(const QByteArray &message)
{
    const int size = message.size();

    int pos = advanceNewlines(message, 0);

    while (pos < size) {
        const int newPos = parseRecord(message, pos);
        if (newPos <= pos) {
            return {pos, true};
        }
        pos = newPos;
    }

    return {pos, false};
}

bool GdbmiParser::isMIRequest(const QString &message)
{
    static const QRegularExpression rx(QStringLiteral(R"--(^\s*(?:\d+\s*)?\-)--"));

    return rx.match(message).hasMatch();
}

std::optional<QString> GdbmiParser::getMICommand(const QString &message)
{
    static const QRegularExpression rx(QStringLiteral(R"--(^\s*(?:\d+\s*)?\-(\S+))--"));

    const auto match = rx.match(message);
    if (!match.hasMatch()) {
        return std::nullopt;
    }
    return match.captured(1);
}

bool GdbmiParser::isMISeparator(const QString &message)
{
    static const QRegularExpression rx(QStringLiteral(R"--(^\s*(gdb)\s*$)--"));

    return rx.match(message).hasMatch();
}

QStringList GdbmiParser::splitCommand(const QString &message)
{
    static const QRegularExpression rx(QStringLiteral(R"--(^(\s*(\d+)\s*)?(\-\S+))--"));

    const auto match = rx.match(message);
    if (!match.hasMatch()) {
        return match.capturedTexts();
    }
    return {message};
}

static int indexOf(const QByteArray &stack, char needle, bool lastIndex)
{
    if (lastIndex) {
        return stack.lastIndexOf(needle);
    }
    return stack.indexOf(needle);
}

int GdbmiParser::splitLines(const QByteArray &buffer, bool lastIndex)
{
    // \r\n
    const int r_idx = indexOf(buffer, '\r', lastIndex);
    if ((r_idx >= 0) && ((r_idx + 1) < buffer.size()) && (buffer.at(r_idx + 1) == '\n')) {
        return r_idx + 1;
    }

    const int n_idx = indexOf(buffer, '\n', lastIndex);
    if (n_idx >= 0) {
        // \n
        return n_idx;
    } else {
        // \r
        return r_idx;
    }
}

} // namespace gdbmi

#include "moc_parser.cpp"
