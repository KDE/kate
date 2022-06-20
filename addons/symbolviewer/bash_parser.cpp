/***************************************************************************
                      bash_parser.cpp  -  description
                             -------------------
    begin                : dec 12 2008
    author               : Daniel Dumitrache
    email                : daniel.dumitrache@gmail.com
 ***************************************************************************/
/***************************************************************************
   SPDX-License-Identifier: GPL-2.0-or-later
***************************************************************************/

#include "plugin_katesymbolviewer.h"

void KatePluginSymbolViewerView::parseBashSymbols(void)
{
    if (!m_mainWindow->activeView()) {
        return;
    }

    QString currline;

    int i;
    // bool mainprog;

    QTreeWidgetItem *node = nullptr;
    QTreeWidgetItem *funcNode = nullptr;
    QTreeWidgetItem *lastFuncNode = nullptr;

    // It is necessary to change names
    m_func->setText(i18n("Show Functions"));

    if (m_treeOn->isChecked()) {
        funcNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Functions")));
        funcNode->setIcon(0, m_icon_function);

        if (m_expandOn->isChecked()) {
            m_symbols->expandItem(funcNode);
        }

        lastFuncNode = funcNode;

        m_symbols->setRootIsDecorated(1);
    } else {
        m_symbols->setRootIsDecorated(0);
    }

    KTextEditor::Document *kDoc = m_mainWindow->activeView()->document();

    static const QRegularExpression function_regexp(QLatin1String("^(function )?([a-zA-Z0-9-_]+) *\\( *\\)"));
    QRegularExpressionMatch match;

    for (i = 0; i < kDoc->lines(); i++) {
        currline = kDoc->line(i);
        currline = currline.trimmed();
        currline = currline.simplified();

        // qDebug(13000)<<currline<<endl;
        if (currline.isEmpty() || currline.at(0) == QLatin1Char('#')) {
            continue;
        }

        // mainprog=false;
        if (m_func->isChecked()) {
            QString funcName;

            match = function_regexp.match(currline);
            if (match.hasMatch()) {
                funcName = match.captured(2);
                funcName.append(QLatin1String("()"));

                if (m_treeOn->isChecked()) {
                    node = new QTreeWidgetItem(funcNode, lastFuncNode);
                    lastFuncNode = node;
                } else {
                    node = new QTreeWidgetItem(m_symbols);
                }

                node->setText(0, funcName);
                node->setIcon(0, m_icon_function);
                node->setText(1, QString::number(i, 10));
            }
        }
    } // for i loop
}
