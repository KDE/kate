// SPDX-FileCopyrightText: 2023 Rémi Peuchot <kde.remi@proton.me>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "test_gdbvariableparser.h"

#include "gdbvariableparser.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QTest>
#include <iostream>
#include <qstringliteral.h>
#include <qtestcase.h>

QTEST_MAIN(TestGdbVariableParser)

// To make the test cases easier to write and read, the small utility 'StructuredOutputFormatter'
// allows to transform several 'GDBVariableParser:variable' signal calls
// into a structured output string.
//
// Example :
// - variable "toto" has flat value "@0x555556ffaff0: {a = {b = 12, c = "hello"}, d=[101]}",
// - 'GDBVariableParser:variable' signal is supposed to be called 5 times
// - 'StructuredOutputFormatter::appendVariable' slot will be called 5 times and construct :
//  toto-->@0x555556ffaff0:
//      a-->{...}
//          b-->12
//          c-->"hello"
//      d-->[101]
class StructuredOutputFormatter : public QObject
{
    Q_OBJECT
public:
    QString getStructuredOutput()
    {
        return m_structuredOutput;
    }

public Q_SLOTS:
    void appendVariable(int parentId, const dap::Variable &variable)
    {
        qDebug() << "appendVariable(parentId: " << parentId << " ; name: " << variable.name << ")";

        QString indentation;
        if (m_variableIndentation.find(parentId) != m_variableIndentation.end()) {
            indentation = m_variableIndentation.at(parentId) + m_indentation;
        }
        m_variableIndentation[variable.variablesReference] = indentation;

        m_structuredOutput.append(indentation);
        m_structuredOutput.append(variable.name);
        m_structuredOutput.append(m_valueIndicator);
        m_structuredOutput.append(variable.value);
        m_structuredOutput.append(m_newLine);
    }

private:
    const QString m_newLine = QStringLiteral("\n");
    const QString m_valueIndicator = QStringLiteral("-->");
    const QString m_indentation = QStringLiteral("    ");
    QString m_structuredOutput;
    std::map<int, QString> m_variableIndentation;
};

void TestGdbVariableParser::parseVariable()
{
    QFETCH(QString, variableName);
    QFETCH(QString, flatVariableValue);
    QFETCH(QString, expectedStructuredOutput);

    StructuredOutputFormatter formatter;
    GDBVariableParser parser;

    connect(&parser, &GDBVariableParser::variable, &formatter, &StructuredOutputFormatter::appendVariable);
    parser.insertVariable(variableName, flatVariableValue, QStringLiteral("unused_type"));
    // QCOMPARE and qDebug truncate the output, use std::cout to print full output
    std::cout << "structuredOutput:\n" << formatter.getStructuredOutput().toStdString() << std::endl;
    std::cout << "expectedStructuredOutput:\n" << expectedStructuredOutput.toStdString() << std::endl;
    QCOMPARE(formatter.getStructuredOutput(), expectedStructuredOutput);
}

void TestGdbVariableParser::parseVariable_data()
{
    QTest::addColumn<QString>("variableName");
    QTest::addColumn<QString>("flatVariableValue");
    QTest::addColumn<QString>("expectedStructuredOutput");

    QTest::newRow("simple_int") << "my_int"
                                << "12"
                                << "my_int-->12\n";

    QTest::newRow("simple_string") << "my_string"
                                   << "\"12\""
                                   << "my_string-->\"12\"\n";

    QTest::newRow("simple_struct") << "my_struct"
                                   << "{b1 = \"aaaa\", b2 = 12, b3 = 1.23456}"
                                   << "my_struct-->{...}\n"
                                      "    b1-->\"aaaa\"\n"
                                      "    b2-->12\n"
                                      "    b3-->1.23456\n";

    QTest::newRow("simple_array") << "my_array"
                                  << "{100, 150, 175}"
                                  << "my_array-->{...}\n"
                                     "    [0]-->100\n"
                                     "    [1]-->150\n"
                                     "    [2]-->175\n";

    QTest::newRow("complex_struct") << "my_struct"
                                    << "{c1 = {b1 = \"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\", b2 = 12, b3 = 1.5}, "
                                       "c2 = {b1 = \"bb{bb}\\\"\\\"\\\\\\\"}}}}}\", b2 = 23, b3 = 2.5}, "
                                       "a = {12, 13, 14}}"
                                    << "my_struct-->{...}\n"
                                       "    c1-->{...}\n"
                                       "        b1-->\"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\"\n"
                                       "        b2-->12\n"
                                       "        b3-->1.5\n"
                                       "    c2-->{...}\n"
                                       "        b1-->\"bb{bb}\\\"\\\"\\\\\\\"}}}}}\"\n"
                                       "        b2-->23\n"
                                       "        b3-->2.5\n"
                                       "    a-->{...}\n"
                                       "        [0]-->12\n"
                                       "        [1]-->13\n"
                                       "        [2]-->14\n";

    QTest::newRow("complex_array") << "my_array"
                                   << "{{d1 = {c1 = {b1 = \"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\", b2 = 12, b3 = 1.5}, "
                                      "c2 = {}, "
                                      "a = {12, 13, 14}}, "
                                      "d2 = {{c1 = {b1 = \"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\", b2 = 12, b3 = 1.5}, "
                                      "c2 = {b1 = \"bb{bb}\\\"\\\"\\\\\\\"}}}}}\", b2 = 23, b3 = 2.5}, "
                                      "a = {12, 13, 14}}, "
                                      "{c1 = {b1 = \"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\", b2 = 12, b3 = 1.5}, "
                                      "c2 = {b1 = \"bb{bb}\\\"\\\"\\\\\\\"}}}}}\", b2 = 23, b3 = 2.5}, "
                                      "a = {12, 13, 14}}}}, "
                                      "{{c1 = {b1 = \"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\", b2 = 12, b3 = 1.5}, "
                                      "c2 = {b1 = \"bb{bb}\\\"\\\"\\\\\\\"}}}}}\", b2 = 23, b3 = 2.5}, "
                                      "a = {12, 13, 14}}, "
                                      "{{c1 = {b1 = \"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\", b2 = 12, b3 = 1.5}, "
                                      "c2 = {}, "
                                      "a = {12, 13, 14}}, "
                                      "{c1 = {b1 = \"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\", b2 = 12, b3 = 1.5}, "
                                      "c2 = {b1 = \"bb{bb}\\\"\\\"\\\\\\\"}}}}}\", b2 = 23, b3 = 2.5}, "
                                      "a = {12, 13, 14}}}}}"
                                   << "my_array-->{...}\n"
                                      "    [0]-->{...}\n"
                                      "        d1-->{...}\n"
                                      "            c1-->{...}\n"
                                      "                b1-->\"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\"\n"
                                      "                b2-->12\n"
                                      "                b3-->1.5\n"
                                      "            c2-->{}\n"
                                      "            a-->{...}\n"
                                      "                [0]-->12\n"
                                      "                [1]-->13\n"
                                      "                [2]-->14\n"
                                      "        d2-->{...}\n"
                                      "            [0]-->{...}\n"
                                      "                c1-->{...}\n"
                                      "                    b1-->\"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\"\n"
                                      "                    b2-->12\n"
                                      "                    b3-->1.5\n"
                                      "                c2-->{...}\n"
                                      "                    b1-->\"bb{bb}\\\"\\\"\\\\\\\"}}}}}\"\n"
                                      "                    b2-->23\n"
                                      "                    b3-->2.5\n"
                                      "                a-->{...}\n"
                                      "                    [0]-->12\n"
                                      "                    [1]-->13\n"
                                      "                    [2]-->14\n"
                                      "            [1]-->{...}\n"
                                      "                c1-->{...}\n"
                                      "                    b1-->\"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\"\n"
                                      "                    b2-->12\n"
                                      "                    b3-->1.5\n"
                                      "                c2-->{...}\n"
                                      "                    b1-->\"bb{bb}\\\"\\\"\\\\\\\"}}}}}\"\n"
                                      "                    b2-->23\n"
                                      "                    b3-->2.5\n"
                                      "                a-->{...}\n"
                                      "                    [0]-->12\n"
                                      "                    [1]-->13\n"
                                      "                    [2]-->14\n"
                                      "    [1]-->{...}\n"
                                      "        [0]-->{...}\n"
                                      "            c1-->{...}\n"
                                      "                b1-->\"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\"\n"
                                      "                b2-->12\n"
                                      "                b3-->1.5\n"
                                      "            c2-->{...}\n"
                                      "                b1-->\"bb{bb}\\\"\\\"\\\\\\\"}}}}}\"\n"
                                      "                b2-->23\n"
                                      "                b3-->2.5\n"
                                      "            a-->{...}\n"
                                      "                [0]-->12\n"
                                      "                [1]-->13\n"
                                      "                [2]-->14\n"
                                      "        [1]-->{...}\n"
                                      "            [0]-->{...}\n"
                                      "                c1-->{...}\n"
                                      "                    b1-->\"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\"\n"
                                      "                    b2-->12\n"
                                      "                    b3-->1.5\n"
                                      "                c2-->{}\n"
                                      "                a-->{...}\n"
                                      "                    [0]-->12\n"
                                      "                    [1]-->13\n"
                                      "                    [2]-->14\n"
                                      "            [1]-->{...}\n"
                                      "                c1-->{...}\n"
                                      "                    b1-->\"ab\\\\{aa\\\"aa\\\\\\\"\\\"}{}}{{{}}aa\"\n"
                                      "                    b2-->12\n"
                                      "                    b3-->1.5\n"
                                      "                c2-->{...}\n"
                                      "                    b1-->\"bb{bb}\\\"\\\"\\\\\\\"}}}}}\"\n"
                                      "                    b2-->23\n"
                                      "                    b3-->2.5\n"
                                      "                a-->{...}\n"
                                      "                    [0]-->12\n"
                                      "                    [1]-->13\n"
                                      "                    [2]-->14\n";

    QTest::newRow("object_with_additional_string_before_opening_brace")
        << "__for_range"
        << "@0x7fff06f96b40: {d = {d = 0x7fff06f96bc0, ptr = 0x7f45550045b5 <findVariableName(QStringView&)+119> u\"blablabla\", size = 1}, static _empty = 0 "
           "u'0000'}"
        << "__for_range-->@0x7fff06f96b40: {...}\n"
           "    d-->{...}\n"
           "        d-->0x7fff06f96bc0\n"
           "        ptr-->0x7f45550045b5 <findVariableName(QStringView&)+119> u\"blablabla\"\n"
           "        size-->1\n"
           "    static _empty-->0 u'0000'\n";
}

#include "moc_test_gdbvariableparser.cpp"
#include "test_gdbvariableparser.moc"
