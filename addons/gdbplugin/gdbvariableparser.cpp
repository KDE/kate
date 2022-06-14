//
// Description: GDB variable parser (refactored from localsview)
//
// SPDX-FileCopyrightText: 2010 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "gdbvariableparser.h"

GDBVariableParser::GDBVariableParser(QObject *parent)
    : QObject(parent)
{
}

void GDBVariableParser::openScope()
{
    m_varId = 0;
    m_allAdded = false;

    Q_EMIT scopeOpened();
}

void GDBVariableParser::closeScope()
{
    m_allAdded = true;

    Q_EMIT scopeClosed();
}

dap::Variable createVariable(const QStringList &symbolAndValue, const int refId)
{
    if (symbolAndValue.count() > 1) {
        return dap::Variable(symbolAndValue[0], symbolAndValue[1], refId);
    } else {
        return dap::Variable(symbolAndValue[0], QString(), refId);
    }
}

void GDBVariableParser::addLocal(const QString &vString)
{
    static const QRegularExpression isValue(QStringLiteral("\\A(\\S*)\\s=\\s(.*)\\z"));
    static const QRegularExpression isStruct(QStringLiteral("\\A\\{\\S*\\s=\\s.*\\z"));
    static const QRegularExpression isStartPartial(QStringLiteral("\\A\\S*\\s=\\s\\S*\\s=\\s\\{\\z"));
    static const QRegularExpression isPrettyQList(QStringLiteral("\\A\\s*\\[\\S*\\]\\s=\\s\\S*\\z"));
    static const QRegularExpression isPrettyValue(QStringLiteral("\\A(\\S*)\\s=\\s(\\S*)\\s=\\s(.*)\\z"));
    static const QRegularExpression isThisValue(QStringLiteral("\\A\\$\\d+\\z"));

    if (m_allAdded) {
        openScope();
    }

    if (vString.isEmpty()) {
        closeScope();
        return;
    }

    QRegularExpressionMatch match = isStartPartial.match(vString);
    if (match.hasMatch()) {
        m_local = vString;
        return;
    }
    match = isPrettyQList.match(vString);
    if (match.hasMatch()) {
        m_local += vString.trimmed();
        if (m_local.endsWith(QLatin1Char(','))) {
            m_local += QLatin1Char(' ');
        }
        return;
    }
    if (vString == QLatin1String("}")) {
        m_local += vString;
    }

    QStringList symbolAndValue;
    QString value;

    if (m_local.isEmpty()) {
        if (vString == QLatin1String("No symbol table info available.")) {
            return; /* this is not an error */
        }
        match = isValue.match(vString);
        if (!match.hasMatch()) {
            qDebug() << "Could not parse:" << vString;
            return;
        }
        symbolAndValue << match.captured(1);
        value = match.captured(2);
        // check out for "print *this"
        match = isThisValue.match(symbolAndValue[0]);
        if (match.hasMatch()) {
            symbolAndValue[0] = QStringLiteral("*this");
        }
    } else {
        match = isPrettyValue.match(m_local);
        if (!match.hasMatch()) {
            qDebug() << "Could not parse:" << m_local;
            m_local.clear();
            return;
        }
        symbolAndValue << match.captured(1) << match.captured(2);
        value = match.captured(3);
    }

    if (value[0] == QLatin1Char('{')) {
        if (value[1] == QLatin1Char('{')) {
            const auto item = createVariable(symbolAndValue, ++m_varId);
            Q_EMIT variable(0, item);
            addArray(item.variablesReference, value.mid(1, value.size() - 2));
        } else {
            match = isStruct.match(value);
            if (match.hasMatch()) {
                const auto item = createVariable(symbolAndValue, ++m_varId);
                Q_EMIT variable(0, item);
                addStruct(item.variablesReference, value.mid(1, value.size() - 2));
            } else {
                Q_EMIT variable(0, dap::Variable(symbolAndValue[0], value));
            }
        }
    } else {
        Q_EMIT variable(0, dap::Variable(symbolAndValue[0], value));
    }

    m_local.clear();
}

void GDBVariableParser::addArray(int parentId, const QString &vString)
{
    // getting here we have this kind of string:
    // "{...}" or "{...}, {...}" or ...
    int count = 1;
    bool inComment = false;
    int index = 0;
    int start = 1;
    int end = 1;

    while (end < vString.size()) {
        if (!inComment) {
            if (vString[end] == QLatin1Char('"')) {
                inComment = true;
            } else if (vString[end] == QLatin1Char('}')) {
                count--;
            } else if (vString[end] == QLatin1Char('{')) {
                count++;
            }
            if (count == 0) {
                QStringList name;
                name << QStringLiteral("[%1]").arg(index);
                index++;
                const auto item = createVariable(name, ++m_varId);
                Q_EMIT variable(parentId, item);
                addStruct(item.variablesReference, vString.mid(start, end - start));
                end += 4; // "}, {"
                start = end;
                count = 1;
            }
        } else {
            if ((vString[end] == QLatin1Char('"')) && (vString[end - 1] != QLatin1Char('\\'))) {
                inComment = false;
            }
        }
        end++;
    }
}

void GDBVariableParser::addStruct(int parentId, const QString &vString)
{
    static const QRegularExpression isArray(QStringLiteral("\\A\\{\\.*\\s=\\s.*\\z"));
    static const QRegularExpression isStruct(QStringLiteral("\\A\\.*\\s=\\s.*\\z"));

    QStringList symbolAndValue;
    QString subValue;
    int start = 0;
    int end;
    while (start < vString.size()) {
        // Symbol
        symbolAndValue.clear();
        end = vString.indexOf(QLatin1String(" = "), start);
        if (end < 0) {
            // error situation -> bail out
            Q_EMIT dap::Variable(QString(), vString.right(start), parentId);
            break;
        }
        symbolAndValue << vString.mid(start, end - start);
        // Value
        start = end + 3;
        end = start;
        if (start < 0 || start >= vString.size()) {
            qDebug() << vString << start;
            break;
        }
        if (vString[start] == QLatin1Char('{')) {
            start++;
            end++;
            int count = 1;
            bool inComment = false;
            // search for the matching }
            while (end < vString.size()) {
                if (!inComment) {
                    if (vString[end] == QLatin1Char('"')) {
                        inComment = true;
                    } else if (vString[end] == QLatin1Char('}')) {
                        count--;
                    } else if (vString[end] == QLatin1Char('{')) {
                        count++;
                    }
                    if (count == 0) {
                        break;
                    }
                } else {
                    if ((vString[end] == QLatin1Char('"')) && (vString[end - 1] != QLatin1Char('\\'))) {
                        inComment = false;
                    }
                }
                end++;
            }
            subValue = vString.mid(start, end - start);
            if (isArray.match(subValue).hasMatch()) {
                const auto item = createVariable(symbolAndValue, ++m_varId);
                Q_EMIT variable(parentId, item);
                addArray(item.variablesReference, subValue);
            } else if (isStruct.match(subValue).hasMatch()) {
                const auto item = createVariable(symbolAndValue, ++m_varId);
                Q_EMIT variable(parentId, item);
                addStruct(item.variablesReference, subValue);
            } else {
                Q_EMIT variable(parentId, dap::Variable(symbolAndValue[0], vString.mid(start, end - start)));
            }
            start = end + 3; // },_
        } else {
            // look for the end of the value in the vString
            bool inComment = false;
            while (end < vString.size()) {
                if (!inComment) {
                    if (vString[end] == QLatin1Char('"')) {
                        inComment = true;
                    } else if (vString[end] == QLatin1Char(',')) {
                        break;
                    }
                } else {
                    if ((vString[end] == QLatin1Char('"')) && (vString[end - 1] != QLatin1Char('\\'))) {
                        inComment = false;
                    }
                }
                end++;
            }
            Q_EMIT variable(parentId, dap::Variable(symbolAndValue[0], vString.mid(start, end - start)));
            start = end + 2; // ,_
        }
    }
}
