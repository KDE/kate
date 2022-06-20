/***************************************************************************
                          ruby_parser.cpp  -  description
                             -------------------
    begin                : May 9th 2007
    author               : 2007 Massimo Callegari
    email                : massimocallegari@yahoo.it
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 ***************************************************************************/
#include "plugin_katesymbolviewer.h"

void KatePluginSymbolViewerView::parseRubySymbols(void)
{
    if (!m_mainWindow->activeView()) {
        return;
    }

    m_macro->setText(i18n("Show Functions"));
    m_struct->setText(i18n("Show Methods"));
    m_func->setText(i18n("Show Classes"));

    QTreeWidgetItem *node = nullptr;
    QTreeWidgetItem *mtdNode = nullptr, *clsNode = nullptr, *functionNode = nullptr;
    QTreeWidgetItem *lastMtdNode = nullptr, *lastClsNode = nullptr;

    KTextEditor::Document *kv = m_mainWindow->activeView()->document();

    if (m_treeOn->isChecked()) {
        clsNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Classes")));
        functionNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Functions")));
        clsNode->setIcon(0, m_icon_class);
        functionNode->setIcon(0, m_icon_function);

        if (m_expandOn->isChecked()) {
            m_symbols->expandItem(clsNode);
            m_symbols->expandItem(functionNode);
        }
        lastClsNode = clsNode;
        mtdNode = clsNode;
        lastMtdNode = clsNode;
        m_symbols->setRootIsDecorated(1);
    } else {
        m_symbols->setRootIsDecorated(0);
    }

    static const QRegularExpression function_regexp(QLatin1String("^(\\s*)def\\s+([a-zA-Z0-9_]+)\\s*(\\(.*\\))"));
    static const QRegularExpression class_regexp(QLatin1String("^class\\s+([a-zA-Z0-9]+)"));
    QRegularExpressionMatch match;

    for (int i = 0; i < kv->lines(); i++) {
        QString cl = kv->line(i);

        match = class_regexp.match(cl);
        if (match.hasMatch()) {
            if (m_func->isChecked()) {
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
                node->setText(0, match.captured(1));
                node->setIcon(0, m_icon_class);
                node->setText(1, QString::number(i, 10));
            }
            continue;
        }

        match = function_regexp.match(cl);
        if (match.hasMatch()) {
            if (m_struct->isChecked() && match.captured(1).isEmpty()) {
                if (m_treeOn->isChecked()) {
                    node = new QTreeWidgetItem(functionNode);
                } else {
                    node = new QTreeWidgetItem(m_symbols);
                }
            } else if (m_macro->isChecked()) {
                if (m_treeOn->isChecked()) {
                    node = new QTreeWidgetItem(mtdNode, lastMtdNode);
                    lastMtdNode = node;
                } else {
                    node = new QTreeWidgetItem(m_symbols);
                }
            }

            node->setToolTip(0, match.captured(2));
            if (m_typesOn->isChecked()) {
                node->setText(0, match.captured(2) + match.captured(3));
            } else {
                node->setText(0, match.captured(2));
            }
            node->setIcon(0, m_icon_function);
            node->setText(1, QString::number(i, 10));
        }
    }
}
