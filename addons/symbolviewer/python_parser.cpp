/***************************************************************************
                           python_parser.cpp  -  description
                             -------------------
    begin                : Apr 2 2003
    author               : 2003 Massimo Callegari
    email                : massimocallegari@yahoo.it
    
    modified             : 2023-05-20 16:25:28
    author               : 2023 Cezar M. Tigaret
    email                : cezar.tigaret@gmail.com
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 ***************************************************************************/
#include "plugin_katesymbolviewer.h"

void KatePluginSymbolViewerView::parsePythonSymbols(void)
{
    if (!m_mainWindow->activeView()) {
        return;
    }

    m_macro->setText(i18n("Show Functions"));
    m_struct->setText(i18n("Show Methods"));
    m_func->setText(i18n("Show Classes"));

    bool commentLine = false;
    bool paramscontinue = false;

    Symbol type;
    QString name;
    QString params;
    QString returnAnnot;
    QString endcolon;
    QString current_class_name;

    QTreeWidgetItem *node = nullptr;
    QTreeWidgetItem *functionNode = nullptr, *mtdNode = nullptr, *clsNode = nullptr;
    QTreeWidgetItem *lastMcrNode = nullptr, *lastMtdNode = nullptr, *lastClsNode = nullptr;

    KTextEditor::Document *kv = m_mainWindow->activeView()->document();

    // kdDebug(13000)<<"Lines counted :"<<kv->numLines()<<endl;
    if (m_treeOn->isChecked()) {
        clsNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Classes")));
        functionNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Functions")));
        functionNode->setIcon(0, m_icon_function);
        clsNode->setIcon(0, m_icon_class);

        if (m_expandOn->isChecked()) {
            m_symbols->expandItem(functionNode);
            m_symbols->expandItem(clsNode);
        }
        lastClsNode = clsNode;
        lastMcrNode = functionNode;
        mtdNode = clsNode;
        lastMtdNode = clsNode;
        m_symbols->setRootIsDecorated(1);
    } else {
        m_symbols->setRootIsDecorated(0);
    }

    static const QChar ret[1] = {0x21b5};
    static const QString contStr(ret, 1);

    static const QRegularExpression comment_regexp(QLatin1String("^[#]"), QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression ml_docsctring_regexp(QLatin1String("\"\"\""), QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression sl_docstring_regexp(QLatin1String("\"\"\"(.*)?\"\"\""), QRegularExpression::UseUnicodePropertiesOption);

    // captures:                                                       1=class name   2=class def params
    static const QRegularExpression class_regexp(QLatin1String("^class ([a-zA-Z0-9_]+)(\\((.*)\\))?( *):"),
                                                 QRegularExpression::UseUnicodePropertiesOption);
    
    static const QRegularExpression function_regexp(
        // captures:    1       2=name          3   4=params+annots                5=return annot
        // QLatin1String("^( *)def ([\\w:\\{\\}!]+)( *)(\\(.*[,;:\\{\\}\\s]*\\)?\\s*)?( -> [\\w.,\\s\\[\\]]+)?( *):$"),
        // captures:    1       2=name          3   4=params+annots                          5=return annot         6   7        
        QLatin1String("^( *)def ([\\w:\\{\\}!]+)( *)(\\(.*[,;:\\/\\{\\[\\]\\}\\s]*\\)?\\s*)?( -> [\\w.,\\s\\[\\]]+)?( *)(:)+$"),
        QRegularExpression::UseUnicodePropertiesOption);
    
    static const QRegularExpression function_regexp2(
        QLatin1String("^( *)def ([\\w:\\{\\}!]+)( *)(\\(.*[,;:\\/\\{\\[\\]\\}\\s]*\\s*)$"),
        QRegularExpression::UseUnicodePropertiesOption
    );

    // captures:                                                            2=func name    3   4=params   5=return annot          6       
    // static const QRegularExpression function_regexp(QLatin1String("^( *)def ([a-zA-Z_0-9]+)( *)(\\(.*\\))?( -> [\\w.,\\s\\[\\]]+)?( *):$"),
    //                                                 QRegularExpression::UseUnicodePropertiesOption);
    // 
    QRegularExpressionMatch match;

    for (int i = 0; i < kv->lines(); i++) {
        int line = i;
        QString cl = kv->line(i);
        QString cl_sp = cl.simplified();
        // concatenate continued lines and remove continuation marker
        if (cl.isEmpty()) {
            continue;
        }
        // while (cl[cl.length() - 1] == QLatin1Char('\\')) {
        //     cl = cl.left(cl.length() - 1);
        //     i++;
        //     if (i < kv->lines()) {
        //         cl += kv->line(i);
        //     } else {
        //         break;
        //     }
        //     if (cl.isEmpty()) {
        //         break;
        //     }
        // }

        // BEGIN skip doc strings """ ... """ or """
        match = ml_docsctring_regexp.match(cl_sp); // match """ anywhere
        
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
        
        // BEGIN determine the symbol type: class or function
        
        match = class_regexp.match(cl_sp); // check for class definition first
        if (match.hasMatch()) {
            type = Symbol::Class; // ⇒ this is a class
            
        } else {
            match = function_regexp.match(cl_sp); // check for function definition
            if (match.hasMatch()) {
                if (match.captured(1).isEmpty() || current_class_name.isEmpty()) // case where function is declared inside a block
                {
                    type = Symbol::Function;
                    current_class_name.clear();
                } else {
                    type = Symbol::Method;
                }
            } else {
                match = function_regexp2.match(cl_sp); 
                if (match.hasMatch()) {
                    if (match.captured(1).isEmpty() || current_class_name.isEmpty()) // case where function is declared inside a block
                    {
                        type = Symbol::Function;
                        current_class_name.clear();
                    } else {
                        type = Symbol::Method;
                    }
                }
                
            }
        }
        // END determine the symbol type: class or function

        if (match.hasMatch()) {
            // if either class or function definition found
            if (type == Symbol::Class) {
                name = match.captured(1);
                params = match.captured(2);
                current_class_name = name;
                
            } else {
                name = match.captured(2);
                params = match.captured(4);
                returnAnnot = match.captured(5);
                if (!returnAnnot.isEmpty()) {
                    params += returnAnnot;
                }
                endcolon = match.captured(7);
                
                paramscontinue = endcolon.isEmpty();
                
                if(paramscontinue) {
                    params += contStr;
                    
                }
            }
            if (m_typesOn->isChecked()) {
                name += params;
            }

            if (m_func->isChecked() && type == Symbol::Class) {
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

            if (m_struct->isChecked() && type == Symbol::Method) {
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

            if (m_macro->isChecked() && type == Symbol::Function) {
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

            name.clear();
            params.clear();
        }
    }
}
