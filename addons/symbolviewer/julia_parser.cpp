/***************************************************************************
                           julia_parser.cpp  -
    Kate parser for Julia files to use with Kate's Symbol viewer plugin.
                             -------------------
    begin                : Feb 25 2023
    author               : 2023 Cezar Tigaret
    email                : cezar.tigaret@gmail.com
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   SPDX-FileCopyrightText: 2023 Cezar M. Tigaret <cezar.tigaret@gmail.com>
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 *                                                                         *
 ***************************************************************************/
#include "plugin_katesymbolviewer.h"

#include <KTextEditor/Document>

enum class Type {
    Function,
    Structure,
    Macro,
    Method
};

void KatePluginSymbolViewerView::parseJuliaSymbols(void)
{
    if (!m_mainWindow->activeView()) {
        return;
    }

    m_macro->setText(i18n("Show Macros"));
    m_struct->setText(i18n("Show Structures"));
    m_func->setText(i18n("Show Functions"));

    bool commentLine = false;
    bool terseFunctionExpresion = false;

    Type type;
    QString name;
    QString module_name;
    QString params;
    QString terseFuncAssignment;
    QString whereStmt;
    QString current_class_name;

    QTreeWidgetItem *node = nullptr;
    QTreeWidgetItem *functionNode = nullptr, *mtdNode = nullptr, *clsNode = nullptr;
    QTreeWidgetItem *lastMcrNode = nullptr, *lastMtdNode = nullptr, *lastClsNode = nullptr;
    QTreeWidgetItem *macroNode = nullptr;
    QTreeWidgetItem *lastMacroNode = nullptr;

    KTextEditor::Document *kv = m_mainWindow->activeView()->document();

    if (m_treeOn->isChecked()) {
        clsNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Structures")));
        functionNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Functions")));
        macroNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Macros")));

        functionNode->setIcon(0, m_icon_function);
        clsNode->setIcon(0, m_icon_class);
        macroNode->setIcon(0, m_icon_context);

        if (m_expandOn->isChecked()) {
            m_symbols->expandItem(functionNode);
            m_symbols->expandItem(clsNode);
            m_symbols->expandItem(macroNode);
        }
        lastClsNode = clsNode;
        lastMcrNode = functionNode;
        lastMacroNode = macroNode;
        mtdNode = clsNode;
        lastMtdNode = clsNode;
        m_symbols->setRootIsDecorated(1);
    } else {
        m_symbols->setRootIsDecorated(0);
    }

    static const QString contStr(QChar(0x21b5));
    // static const QString contStr(0x21b5);

    static const QRegularExpression comment_regexp(QStringLiteral("[#]"), QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression ml_docsctring_regexp(QStringLiteral("\"\"\""), QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression sl_docstring_regexp(QStringLiteral("\"\"\"(.*)?\"\"\""), QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression class_regexp(QStringLiteral("(@[a-zA-Z0-9_\\s]+)?(?:struct|mutable\\s+struct)\\s+([\\w!a-zA-Z0-9_.]+)"),
                                                 QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression function_regexp(
        QStringLiteral("(@[a-zA-Z0-9_\\s]+)?function\\s+([\\w:!.]+)\\s*(\\(.*[,;:\\{\\}\\s]*\\)?\\s*)?$( where [\\w:<>=.\\{\\}]*\\s?$)?"),
        QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression terse_function_regexp(
        QStringLiteral(
            "^(@[a-zA-Z0-9_\\s]+ )?([\\w:.]+)?([\\w:\\{\\}!]+)(\\s?)(\\(.*[\\),;\\s]*\\))(\\s?)(where [\\w:<>.,\\s\\{\\}a-zA-Z0-9]*\\s?)?(\\s?)\\s*=\\s*(.*)$"),
        QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression macro_regexp(QStringLiteral("^macro ([\\w:\\{\\}!]+)(\\s*)(\\(.*\\))?"), QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression assert_regexp(QStringLiteral("@assert"));

    QRegularExpressionMatch match;

    for (int i = 0; i < kv->lines(); i++) {
        int line = i;
        int indexOfHash = -1;
        QString cl = kv->line(i);
        if (cl.isEmpty()) {
            continue;
        }

        // concatenate continued lines and remove continuation marker (from python_parser.cpp)
        while (cl[cl.length() - 1] == QLatin1Char('\\')) {
            cl = cl.left(cl.length() - 1);
            i++;
            if (i < kv->lines()) {
                cl += kv->line(i);
            } else {
                break;
            }
            if (cl.isEmpty()) {
                break;
            }
        }

        QString cl_sp = cl.simplified();

        // strip away comments
        indexOfHash = cl_sp.indexOf(QLatin1Char('#'));
        if (indexOfHash > 0) {
            cl_sp = cl_sp.left(indexOfHash - 1);
        } else if (indexOfHash == 0) {
            continue;
        }
        // skip asserts
        match = assert_regexp.match(cl_sp);

        if (match.hasMatch()) {
            continue;
        }

        // skip # comments
        match = comment_regexp.match(cl_sp);

        if (match.hasMatch()) {
            continue;
        }

        // BEGIN skip doc strings """ ... """ or """
        // """
        //
        match = ml_docsctring_regexp.match(cl_sp); // match """ anywhere

        if (match.hasMatch()) {
            match = sl_docstring_regexp.match(cl_sp);
            if (match.hasMatch()) {
                commentLine = false;
                continue;

            } else {
                commentLine = !commentLine;
                continue;
            }
        } else {
            match = sl_docstring_regexp.match(cl_sp);
            if (match.hasMatch()) {
                commentLine = false;
                continue;
            }
        }

        if (commentLine) {
            continue;
        }
        // END skip doc strings (""" ... """)

        terseFunctionExpresion = false;

        whereStmt.clear();

        match = class_regexp.match(cl_sp);

        if (match.hasMatch()) {
            type = Type::Structure;

        } else {
            match = macro_regexp.match(cl_sp);
            if (match.hasMatch()) {
                type = Type::Macro;
            } else {
                match = function_regexp.match(cl_sp);
                if (match.hasMatch()) {
                    type = Type::Function;
                } else {
                    match = terse_function_regexp.match(cl_sp);
                    if (match.hasMatch()) {
                        type = Type::Function;
                        terseFunctionExpresion = true;
                    } else {
                        continue;
                    }
                }
            }
        }

        if (match.hasMatch()) {
            // either Structure, Macro, or Function egexp have matched
            if (type == Type::Structure) {
                name = match.captured(2);
                current_class_name = name;
                params.clear();
            } else {
                if (type == Type::Function) {
                    if (terseFunctionExpresion) {
                        terseFuncAssignment = match.captured(9);
                        whereStmt = match.captured(7);
                        if (!terseFuncAssignment.isEmpty()) {
                            module_name = match.captured(2); // useful when overloading a function from other module (in Julia that's typically 'adding' a
                                                             // method to the overloaded function <-> function dispatch mechanism)
                            name = match.captured(3);
                            if (!module_name.isEmpty()) {
                                name = module_name + name;
                            }
                            params = match.captured(5);

                            if (!whereStmt.isEmpty()) {
                                params += QLatin1String(" ");
                                params += whereStmt;
                            }

                        } else {
                            continue;
                        }

                    } else {
                        name = match.captured(2);
                        params = match.captured(3);
                        whereStmt = match.captured(4);
                    }

                } else if (type == Type::Macro) {
                    name = match.captured(1);
                    params = match.captured(3);
                } else {
                    continue;
                }

                if (!params.isEmpty() && !params.endsWith(QLatin1String(")"))) {
                    if (!terseFunctionExpresion) {
                        if (whereStmt.isEmpty() && !params.contains(QLatin1String("where"))) {
                            params += QLatin1Char(' ');
                            params += contStr;

                        } else {
                            params += QLatin1String(" ");
                            params += whereStmt;
                            whereStmt.clear();
                        }
                    }
                }
            }

            if (m_typesOn->isChecked()) {
                name += params;
            }

            if (m_func->isChecked() && type == Type::Structure) {
                if (m_treeOn->isChecked()) {
                    node = new QTreeWidgetItem(clsNode, lastClsNode);
                    if (m_expandOn->isChecked()) {
                        m_symbols->expandItem(node);
                    }
                    lastClsNode = node;
                    mtdNode = lastClsNode;
                    lastMtdNode = lastClsNode;
                } else {
                    node = new QTreeWidgetItem(m_symbols);
                }

                node->setText(0, name);
                node->setIcon(0, m_icon_class);
                node->setText(1, QString::number(line, 10));
            }

            if (m_struct->isChecked() && type == Type::Method) {
                if (m_treeOn->isChecked()) {
                    node = new QTreeWidgetItem(mtdNode, lastMtdNode);
                    lastMtdNode = node;
                } else {
                    node = new QTreeWidgetItem(m_symbols);
                }

                node->setText(0, name);
                node->setIcon(0, m_icon_function);
                node->setText(1, QString::number(line, 10));
            }

            if (m_macro->isChecked() && type == Type::Function) {
                if (m_treeOn->isChecked()) {
                    node = new QTreeWidgetItem(functionNode, lastMcrNode);
                    lastMcrNode = node;
                } else {
                    node = new QTreeWidgetItem(m_symbols);
                }

                node->setText(0, name);
                node->setIcon(0, m_icon_function);
                node->setText(1, QString::number(line, 10));
            }

            if (m_macro->isChecked() && type == Type::Macro) {
                if (m_treeOn->isChecked()) {
                    node = new QTreeWidgetItem(macroNode, lastMacroNode);
                    lastMacroNode = node;
                } else {
                    node = new QTreeWidgetItem(m_symbols);
                }

                node->setText(0, name);
                node->setIcon(0, m_icon_function);
                node->setText(1, QString::number(line, 10));
            }

            name.clear();
            params.clear();
        }
    }
}
