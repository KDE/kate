//
// Description: GDB variable parser
//
// This class parses a "flat" GDB variable string and outputs the structure of its child variables.
// A signal is emitted for each of its child variable with a reference to its parent variable.
//
// Example : GDB gives the following flat value for the variable 'abcd':
// "{name = \"hello\", d = {a = 0x7fffe0020ff0, size = 5}, value = 12}"
//
// The output will be the following structure :
//
// Symbol            Value
// ----------------------------------
// abcd
//   name          "hello"
//     d
//       a          0x7fffe0020ff0
//       size       5
//     value        12
//
// SPDX-FileCopyrightText: 2010 Kåre Särs <kare.sars@iki.fi>
// SPDX-FileCopyrightText: 2023 Rémi Peuchot <kde.remi@proton.me>
//
// SPDX-License-Identifier: LGPL-2.0-only

#include "gdbvariableparser.h"

GDBVariableParser::GDBVariableParser(QObject *parent)
    : QObject(parent)
{
}

void GDBVariableParser::insertVariable(const QString &name, const QString &value, const QString &type, bool changed)
{
    QStringView tail(value);
    insertNamedVariable(0, name, 0, tail, type, changed);
}

// Take the next available variable id, signal the variable and return its id
int GDBVariableParser::createAndSignalVariable(int parentId, const QStringView name, const QStringView value, const QString &type, bool changed)
{
    m_variableId++;
    dap::Variable var(name.toString(), value.toString(), m_variableId);
    var.valueChanged = changed;
    if (!type.isEmpty()) {
        var.type = type;
    }
    Q_EMIT variable(parentId, var);
    return m_variableId;
}

// Return the first index of the given char outside of quoted strings
// example : firstIndexOf("aa\"blabla\"hello", 'l') gives 12 (the first 'l' of "hello")
// Note : the quoted strings can also contain escaped quotes
// example : firstIndexOf("aa\"bla\\\"bl\\\"a\"hello", 'l') gives 14 (the first 'l' of "hello")
qsizetype firstIndexOf(const QStringView tail, QChar ch)
{
    const QChar QUOTE((short)'"');
    const QChar BACKSLASH((short)'\\');
    QChar previous(0);
    bool inQuotedString = false;
    for (int i = 0; i < tail.length(); i++) {
        QChar current = tail[i];
        if (inQuotedString) {
            if (current == QUOTE && previous != BACKSLASH) {
                inQuotedString = false;
            }
        } else {
            if (current == ch) {
                return i;
            }
            if (current == QUOTE) {
                inQuotedString = true;
            }
        }
        previous = current;
    }
    return -1;
}

// Return index of first char among 'characters' in tail
qsizetype firstIndexOf(const QStringView tail, QString characters)
{
    qsizetype first = -1;
    for (auto ch : characters) {
        qsizetype i = firstIndexOf(tail, ch);
        if (i != -1 && (first == -1 || i < first)) {
            first = i;
        }
    }
    return first;
}

// If tail starts with pattern "name = value" :
// - advance tail to first char of value
// - extract and return name
// else :
// - let tail unchanged
// - return empty string
QStringView findVariableName(QStringView &tail)
{
    const QChar EQUAL((short)'=');
    auto closingIndex = firstIndexOf(tail, QStringLiteral("=,{}"));
    if (closingIndex != -1 && tail[closingIndex] == EQUAL) {
        QStringView name = tail.mid(0, closingIndex).trimmed();
        tail = tail.mid(closingIndex + 1).trimmed();
        return name;
    }
    return QStringView();
}

// Parse the (eventually named) variable at the beginning of the given tail.
// The pattern defines the value type :
// - "name = value" : it's a named variable, 'itemIndex' is ignored
// - "value" : it's an array item of index 'itemIndex'
// The given tail will be advanced to the next character after this variable value.
void GDBVariableParser::insertVariable(int parentId, int itemIndex, QStringView &tail, const QString &type, bool changed)
{
    QStringView name = findVariableName(tail);
    if (name.isEmpty()) {
        // No variable name : it's an array item
        QString itemName = QStringLiteral("[%1]").arg(itemIndex);
        insertNamedVariable(parentId, itemName, itemIndex, tail, type, changed);
        return;
    }
    insertNamedVariable(parentId, name, itemIndex, tail, type, changed);
}

// Parse the variable value at the beginning of the given tail.
// The pattern defines the value type :
// - if value contains an open brace, it's a parent object with children : "optional_string{child0, child1, ...}"
// - else, it's a simple variable value
// The given tail will be advanced to the next character after this variable value.
void GDBVariableParser::insertNamedVariable(int parentId, QStringView name, int itemIndex, QStringView &tail, const QString &type, bool changed)
{
    // Find an opening brace in the value (before next comma or next closing brace)
    const QChar OPENING_BRACE((short)'{');
    int openingIndex = firstIndexOf(tail, QStringLiteral(",{}"));
    if (openingIndex != -1 && tail[openingIndex] == OPENING_BRACE) {
        // It's a parent object
        QString value = tail.mid(0, openingIndex).toString();
        tail = tail.mid(openingIndex + 1).trimmed(); // Advance after the opening brace

        if (tail.startsWith(QStringLiteral("}"))) {
            // It's an empty object
            value = value + QStringLiteral("{}");
            createAndSignalVariable(parentId, name, value, type, changed);
        } else {
            // It contains some child variables
            value = value + QStringLiteral("{...}");

            // Create the parent object
            int id = createAndSignalVariable(parentId, name, value, type, changed);

            // Insert the first child variable, siblings will be created recursively
            insertVariable(id, 0, tail, QStringLiteral(""), changed);
        }

        // All child variables have been parsed, the parent is supposed to be closed now
        if (tail.startsWith(QStringLiteral("}"))) {
            tail = tail.mid(1).trimmed(); // Advance after the closing brace
        } else {
            qWarning() << "Missing closing brace at the end of parent variable";
        }
    } else {
        // It's a simple variable value
        auto valueLength = firstIndexOf(tail, QStringLiteral(",}"));
        if (valueLength == -1) {
            // It's the last value in the tail : take everything else
            valueLength = tail.length();
        }

        // Extract the value
        QStringView value = tail.mid(0, valueLength).trimmed();
        createAndSignalVariable(parentId, name, value, QStringLiteral(""), changed);
        tail = tail.mid(valueLength).trimmed(); // Advance after value
    }

    // Variable has been parsed, eventually parse its next sibling
    if (tail.startsWith(QStringLiteral(","))) {
        // There is a sibling
        tail = tail.mid(1).trimmed(); // Advance after the comma
        insertVariable(parentId, itemIndex + 1, tail, QStringLiteral(""), changed);
    }
}

#include "moc_gdbvariableparser.cpp"
