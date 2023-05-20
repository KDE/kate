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
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 ***************************************************************************/
#include "plugin_katesymbolviewer.h"

enum class Type { Function, Structure, Macro, Method };

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
    // bool inTry = false;
    // bool inIf = false;
    // bool inWhile = false;
    // bool inFor = false;
    // bool inBegin = false;
    // bool inFunctionBlock = false;
    // bool inMacroBlock = false;

    Type type;
    QString name;
    QString module_name;
    QString params;
    QString paramsSubstr;
    QString terseFuncAssignment;
    QString whereStmt;
    QString current_class_name;
    QString mutable_kw;

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

    static const QChar ret[1] = {0x21b5};
    static const QString contStr(ret, 1);

    static const QRegularExpression comment_regexp(QLatin1String("[#]"), QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression ml_docsctring_regexp(QLatin1String("\"\"\""), QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression sl_docstring_regexp(QLatin1String("\"\"\"(.*)?\"\"\""), QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression class_regexp(QLatin1String("(@[a-zA-Z0-9_\\s]+)?(?:struct|mutable\\s+struct)\\s+([\\w!a-zA-Z0-9_.]+)"),
                                                 QRegularExpression::UseUnicodePropertiesOption);
    
    // static const QRegularExpression try_expr(QLatin1String("try"), QRegularExpression::UseUnicodePropertiesOption);
    // static const QRegularExpression if_expr(QLatin1String("if"), QRegularExpression::UseUnicodePropertiesOption);
    // static const QRegularExpression while_expr(QLatin1String("while"), QRegularExpression::UseUnicodePropertiesOption);
    // static const QRegularExpression for_expr(QLatin1String("for"), QRegularExpression::UseUnicodePropertiesOption);
    // static const QRegularExpression begin_expr(QLatin1String("begin"), QRegularExpression::UseUnicodePropertiesOption);
    // static const QRegularExpression eol_end_expr(QLatin1String("end$"), QRegularExpression::UseUnicodePropertiesOption);
    // static const QRegularExpression end_expr(QLatin1String("end"), QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression function_regexp(
        // captures:    1=@qualif                      2=name               3=params+annots                  4=whereStmt
        QLatin1String("(@[a-zA-Z0-9_\\s]+)?function\\s+([\\w:!.]+)\\s*(\\(.*[,;:\\{\\}\\s]*\\)?\\s*)?$( where [\\w:<>=.\\{\\}]*\\s?$)?"),
        QRegularExpression::UseUnicodePropertiesOption);
    // static const QRegularExpression function_regexp(QLatin1String("^(@[a-zA-Z0-9_\\s]+)?function ([\\w:\\{\\}!]+)(\\(.*[,;:\\{\\}\\s]*\\)?\\s*)$( where
    // [\\w:<>=\\s\\(\\)\\{\\}]\\s?$)?"), QRegularExpression::UseUnicodePropertiesOption);

    // static const QRegularExpression terse_function_regexp(
    //     // captures:   1=@qualif                 2=name                          3=params+annots               4=whereStmt                      5=statement
    //     QLatin1String("^(@[a-zA-Z0-9_\\s]+)?\\s+([\\w:!.a-zA-Z0-9_]+)\\s*(\\(.*[,;:\\{\\}\\s]*\\)?\\s*)( where [\\w:<>.\\{\\}]*\\s?)?\\s*=\\s*(.*)?"),
    //     QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression terse_function_regexp(
        // captures:   1=@qualif             2         3               4     5                    6     7                       8     9     10    11
        QLatin1String("(@[a-zA-Z0-9_\\s]+ )?([\\w:.]+)?([\\w:\\{\\}!]+)(\\s?)(\\(.*[\\),;\\s]*\\))(\\s?)(where [\\w:<>=.]*\\s?)?(\\s?)(=){1}(\\s*)(\\(.*\\))?"),
        QRegularExpression::UseUnicodePropertiesOption);

    // static const QRegularExpression terse_function_regexp(QLatin1String("(@[a-zA-Z0-9_\\s]+
    // )?([\\w:.]+)?([\\w:\\{\\}!]+)(\\s?)(\\(.*[\\),;\\s]*\\))(\\s?)(where [\\w:<>=\\s\\(\\)\\{\\}]\\s?)?(\\s?)(=){1}(\\s*)(\\(.*\\))?"),
    // QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression macro_regexp(QLatin1String("^macro ([\\w:\\{\\}!]+)(\\s*)(\\(.*\\))?"), QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression assert_regexp(QLatin1String("@assert"));

    QRegularExpressionMatch match; //, match1;

    for (int i = 0; i < kv->lines(); i++) {
        int line = i;
        QString cl = kv->line(i);
        QString cl_sp = cl.simplified();
        // QString cl_tr = cl.trimmed();

        // qDebug() << "line " << line+1 << cl << Qt::endl;
        
        // concatenate continued lines and remove continuation marker (from python_parser.cpp)
        if (cl.isEmpty()) {
            continue;
        }
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
        // match1 = sl_docstring_regexp.match(cl.simplified()); // also match """ <stuff> """ triplet

        if (match.hasMatch()) {
            match = sl_docstring_regexp.match(cl_sp);
            if (match.hasMatch()) {
                // qDebug() << "line " << line+1 << " sl match " << cl << Qt::endl;
                commentLine = false;
                continue;

            } else {
                // qDebug() << "line " << line+1 << " ml match " << "commentLine " << commentLine << " -> " << !commentLine << " " << cl << Qt::endl;
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

        if (match.hasMatch()) { // try and  match struct first
            type = Type::Structure;
            
        } else {
            match = macro_regexp.match(cl_sp); // finally , try to match a macro definition NOTE/TODO: terse macro definitions?
            if (match.hasMatch()) {
                type = Type::Macro;
            } else {
                // continue;
                match = function_regexp.match(cl_sp); // the try and match verbose function definition
                if (match.hasMatch()) {
                    type = Type::Function;

                } else {
                    // continue;
                    match = terse_function_regexp.match(cl_sp); // try to match a terse function definition
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
                // name=match.captured(5);
                current_class_name = name;
                params.clear();
            } else {
                if (type == Type::Function) {
                    if (terseFunctionExpresion) {
                        // name = match.captured(2);
                        // params = match.captured(3);
                        // whereStmt = match.captured(4);
                        
                        
                        terseFuncAssignment = match.captured(9);
                        // qDebug() << "line " << line+1 << " terseFuncAssignment " << terseFuncAssignment << Qt::endl;
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
                            // qDebug() << "line " << line+1 << " empty terseFuncAssignment " << cl << Qt::endl;
                            continue;
                        }

                    } else {
                        name = match.captured(2);
                        params = match.captured(3);
                        whereStmt = match.captured(4);
                    }
                    // qDebug() << "line " << line+1 << " name " << name << " params "<< params << " whereStmt " << whereStmt << Qt::endl;

                } else if (type == Type::Macro) {
                    name = match.captured(1);
                    params = match.captured(3);
                } else {
                    continue;
                }

                if (!params.isEmpty() & !params.endsWith(QLatin1String(")"))) {
                    // qDebug() << "line " << line+1 << " params = "<< params << " whereStmt = " << whereStmt << Qt::endl;
                    if (!terseFunctionExpresion) {
                        // if (!whereStmt.isEmpty() | params.contains(QLatin1String("where"))) {
                        //     params += QLatin1String(" ");
                        //     params += whereStmt;
                        //     whereStmt.clear();
                        // }
                        if (whereStmt.isEmpty() & !params.contains(QLatin1String("where"))) {
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
