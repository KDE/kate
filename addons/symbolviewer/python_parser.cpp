/***************************************************************************
                           python_parser.cpp  -  description
                             -------------------
    begin                : Apr 2 2003
    author               : 2003 Massimo Callegari
    email                : massimocallegari@yahoo.it
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

    Symbol type;
    QString name;
    QString params;
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

    static const QRegularExpression class_regexp(QLatin1String("^class ([a-zA-Z0-9_]+)(\\((.*)\\))?:"));
    static const QRegularExpression function_regexp(QLatin1String("^( *)def ([a-zA-Z_0-9]+)(\\(.*\\))?:"));
    QRegularExpressionMatch match;
    for (int i = 0; i < kv->lines(); i++) {
        int line = i;
        QString cl = kv->line(i);
        // concatenate continued lines and remove continuation marker
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

        match = class_regexp.match(cl);
        if (match.hasMatch()) {
            type = Symbol::Class;
        } else {
            match = function_regexp.match(cl);
            if (match.hasMatch()) {
                if (match.captured(1).isEmpty() || current_class_name.isEmpty() // case where function is declared inside a block
                ) {
                    type = Symbol::Function;
                    current_class_name.clear();
                } else {
                    type = Symbol::Method;
                }
            }
        }

        if (match.hasMatch()) {
            if (type == Symbol::Class) {
                name = match.captured(1);
                params = match.captured(2);
                current_class_name = name;
            } else {
                name = match.captured(2);
                params = match.captured(3);
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
