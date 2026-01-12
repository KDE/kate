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

    QString name;
    QString params;
    QString returnAnnot;
    QString endcolon;
    QString current_class_name;

    QTreeWidgetItem *node = nullptr;
    QTreeWidgetItem *functionNode = nullptr, *mtdNode = nullptr, *clsNode = nullptr;
    QTreeWidgetItem *lastMcrNode = nullptr, *lastMtdNode = nullptr, *lastClsNode = nullptr;

    KTextEditor::Document *kv = m_mainWindow->activeView()->document();

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

    // static const QString contStr(0x21b5);
    static const QString contStr(QChar(0x21b5));

    // static const QRegularExpression comment_regexp(QLatin1String("^[#]"), QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression ml_docsctring_regexp(QLatin1String("\"\"\""), QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression sl_docstring_regexp(QLatin1String("\"\"\"(.*)?\"\"\""), QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression class_regexp(QLatin1String("^class ([\\w]+)\\s*(\\([\\w.,\\s]*\\)?)?\\s*(:$)?"),
                                                 QRegularExpression::UseUnicodePropertiesOption);

    static const QRegularExpression function_regexp(
        QLatin1String("^([\t ]*)def\\s+([\\w]+)\\s*(\\([\\w,;.:*=\\/\\[\\]\\s\"\']*\\)?)?\\s*(-> [\\w.,\\s\\[\\]]+)?\\s*(:$)?"),
        QRegularExpression::UseUnicodePropertiesOption);

    QRegularExpressionMatch match;

    for (int i = 0; i < kv->lines(); i++) {
        int line = i;
        QString cl = kv->line(i);
        if (cl.isEmpty()) {
            continue;
        }
        // concatenate continued lines and remove continuation marker
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

        match = ml_docsctring_regexp.match(cl);

        if (match.hasMatch()) {
            match = sl_docstring_regexp.match(cl);
            if (match.hasMatch()) {
                commentLine = false;
                continue;

            } else {
                commentLine = !commentLine;
                continue;
            }
        } else {
            match = sl_docstring_regexp.match(cl);
            if (match.hasMatch()) {
                commentLine = false;
                continue;
            }
        }

        if (commentLine) {
            continue;
        }

        Symbol type;
        match = class_regexp.match(cl);
        if (match.hasMatch()) {
            type = Symbol::Class;
        } else {
            match = function_regexp.match(cl);
            if (match.hasMatch()) {
                if (match.captured(1).isEmpty() || current_class_name.isEmpty()) // case where function is declared inside a block
                {
                    type = Symbol::Function;
                    current_class_name.clear();
                } else {
                    type = Symbol::Method;
                }
            } else {
                // nothing to do in this iteration
                continue;
            }
        }

        // if either class or function definition found
        if (type == Symbol::Class) {
            name = match.captured(1);
            params = match.captured(2);
            endcolon = match.captured(3);
            current_class_name = name;

            if (endcolon.isEmpty()) {
                params += QLatin1Char(' ');
                params += contStr;
            }

        } else {
            name = match.captured(2);
            params = match.captured(3);
            returnAnnot = match.captured(4);
            endcolon = match.captured(5);

            if (!returnAnnot.isEmpty()) {
                params += QLatin1Char(' ');
                params += returnAnnot;
            }

            if (endcolon.isEmpty()) {
                params += QLatin1Char(' ');
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
