/***************************************************************************
                          php_parser.cpp  -  description
                             -------------------
    begin         : Apr 1st 2007
    last update   : Sep 14th 2010
    author(s)     : 2007, Massimo Callegari <massimocallegari@yahoo.it>
                  : 2010, Emmanuel Bouthenot <kolter@openics.org>
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 ***************************************************************************/

#include "plugin_katesymbolviewer.h"
#include <QRegularExpression>

void KatePluginSymbolViewerView::parsePhpSymbols(void)
{
    if (!m_mainWindow->activeView()) {
        return;
    }

    QTreeWidgetItem *node = nullptr;
    QTreeWidgetItem *namespaceNode = nullptr, *defineNode = nullptr, *classNode = nullptr, *functionNode = nullptr;
    QTreeWidgetItem *lastNamespaceNode = nullptr, *lastDefineNode = nullptr, *lastClassNode = nullptr, *lastFunctionNode = nullptr;

    KTextEditor::Document *kv = m_mainWindow->activeView()->document();

    if (m_treeOn->isChecked()) {
        namespaceNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Namespaces")));
        defineNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Defines")));
        classNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Classes")));
        functionNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Functions")));

        namespaceNode->setIcon(0, m_icon_context);
        defineNode->setIcon(0, m_icon_typedef);
        classNode->setIcon(0, m_icon_class);
        functionNode->setIcon(0, m_icon_function);

        if (m_expandOn->isChecked()) {
            m_symbols->expandItem(namespaceNode);
            m_symbols->expandItem(defineNode);
            m_symbols->expandItem(classNode);
            m_symbols->expandItem(functionNode);
        }

        lastNamespaceNode = namespaceNode;
        lastDefineNode = defineNode;
        lastClassNode = classNode;
        lastFunctionNode = functionNode;

        m_symbols->setRootIsDecorated(1);
    } else {
        m_symbols->setRootIsDecorated(0);
    }

    // Namespaces: https://www.php.net/manual/en/language.namespaces.php
    static const QRegularExpression namespaceRegExp(QLatin1String("^namespace\\s+([^;\\s]+)"), QRegularExpression::CaseInsensitiveOption);
    // defines: https://www.php.net/manual/en/function.define.php
    static const QRegularExpression defineRegExp(QLatin1String("(^|\\W)define\\s*\\(\\s*['\"]([^'\"]+)['\"]"), QRegularExpression::CaseInsensitiveOption);
    // classes: https://www.php.net/manual/en/language.oop5.php
    static const QRegularExpression classRegExp(
        QLatin1String("^((abstract\\s+|final\\s+)?)class\\s+([\\w_][\\w\\d_]*)\\s*(implements\\s+[\\w\\d_]*|extends\\s+[\\w\\d_]*)?"),
        QRegularExpression::CaseInsensitiveOption);
    // interfaces: https://www.php.net/manual/en/language.oop5.interfaces.php
    static const QRegularExpression interfaceRegExp(QLatin1String("^interface\\s+([\\w_][\\w\\d_]*)"), QRegularExpression::CaseInsensitiveOption);
    // traits: https://www.php.net/manual/en/language.oop5.traits.php
    static const QRegularExpression traitRegExp(QLatin1String("^trait\\s+([\\w_][\\w\\d_]*)"), QRegularExpression::CaseInsensitiveOption);
    // classes constants: https://www.php.net/manual/en/language.oop5.constants.php
    static const QRegularExpression constantRegExp(QLatin1String("^const\\s+([\\w_][\\w\\d_]*)"), QRegularExpression::CaseInsensitiveOption);
    // functions: https://www.php.net/manual/en/language.oop5.constants.php
    static const QRegularExpression functionRegExp(
        QLatin1String("^(\\s*)((public|protected|private)?(\\s*static)?\\s+)?function\\s+&?\\s*([\\w_][\\w\\d_]*)\\s*(.*)$"),
        QRegularExpression::CaseInsensitiveOption);
    // variables: https://www.php.net/manual/en/language.oop5.properties.php
    static const QRegularExpression varRegExp(QLatin1String("^((var|public|protected|private)?(\\s*static)?\\s+)?\\$([\\w_][\\w\\d_]*)"),
                                              QRegularExpression::CaseInsensitiveOption);

    // function args detection: “function a($b, $c=null)” => “$b, $v”
    static const QRegularExpression functionArgsRegExp(QLatin1String("(\\$[\\w_]+)"), QRegularExpression::CaseInsensitiveOption);
    QStringList functionArgsList;
    QString nameWithTypes;

    // replace literals by empty strings: “function a($b='nothing', $c="pretty \"cool\" string")” => “function ($b='', $c="")”
    static const QRegularExpression literalRegExp(QLatin1String("([\"'])(?:\\\\.|[^\\\\])*\\1"), QRegularExpression::InvertedGreedinessOption);
    // remove useless comments: “public/* static */ function a($b, $c=null) /* test */” => “public function a($b, $c=null)”
    static const QRegularExpression blockCommentInline(QLatin1String("/\\*.*\\*/"), QRegularExpression::InvertedGreedinessOption);

    QRegularExpressionMatch match, matchClass, matchInterface, matchTrait, matchFunctionArg;
    QRegularExpressionMatchIterator matchFunctionArgs;

    bool inBlockComment = false;
    bool inClass = false, inFunction = false;

    // QString debugBuffer("SymbolViewer(PHP), line %1 %2 → [%3]");

    for (int i = 0; i < kv->lines(); i++) {
        // kdDebug(13000) << debugBuffer.arg(i, 4).arg("=origin", 10).arg(kv->line(i));

        // keeping a copy of the line without any processing
        QString realLine = kv->line(i);

        QString line = realLine.simplified();

        // kdDebug(13000) << debugBuffer.arg(i, 4).arg("+simplified", 10).arg(line);

        // keeping a copy with literals for catching “defines()”
        QString lineWithliterals = line;

        // reduce literals to empty strings to not match comments separators in literals
        line.replace(literalRegExp, QLatin1String("\\1\\1"));

        // kdDebug(13000) << debugBuffer.arg(i, 4).arg("-literals", 10).arg(line);
        line.remove(blockCommentInline);

        // kdDebug(13000) << debugBuffer.arg(i, 4).arg("-comments", 10).arg(line);

        // trying to find comments and to remove commented parts
        if (const int pos = line.indexOf(QLatin1Char('#')); pos >= 0) {
            line.truncate(pos);
        }
        if (const int pos = line.indexOf(QLatin1String("//")); pos >= 0) {
            line.truncate(pos);
        }
        if (const int pos = line.indexOf(QLatin1String("/*")); pos >= 0) {
            line.truncate(pos);
            inBlockComment = true;
        }
        if (const int pos = line.indexOf(QLatin1String("*/")); pos >= 0) {
            line = line.right(line.length() - pos - 2);
            inBlockComment = false;
        }

        if (inBlockComment) {
            continue;
        }

        // trimming again after having removed the comments
        line = line.simplified();
        // kdDebug(13000) << debugBuffer.arg(i, 4).arg("+simplified", 10).arg(line);

        // detect NameSpaces
        match = namespaceRegExp.match(line);
        if (match.hasMatch()) {
            if (m_treeOn->isChecked()) {
                node = new QTreeWidgetItem(namespaceNode, lastNamespaceNode);
                if (m_expandOn->isChecked()) {
                    m_symbols->expandItem(node);
                }
                lastNamespaceNode = node;
            } else {
                node = new QTreeWidgetItem(m_symbols);
            }
            node->setText(0, match.captured(1));
            node->setIcon(0, m_icon_context);
            node->setText(1, QString::number(i, 10));
        }

        // detect defines
        match = defineRegExp.match(lineWithliterals);
        if (match.hasMatch()) {
            if (m_treeOn->isChecked()) {
                node = new QTreeWidgetItem(defineNode, lastDefineNode);
                lastDefineNode = node;
            } else {
                node = new QTreeWidgetItem(m_symbols);
            }
            node->setText(0, match.captured(2));
            node->setIcon(0, m_icon_typedef);
            node->setText(1, QString::number(i, 10));
        }

        // detect classes, interfaces and trait
        matchClass = classRegExp.match(line);
        matchInterface = interfaceRegExp.match(line);
        matchTrait = traitRegExp.match(line);
        if (matchClass.hasMatch() || matchInterface.hasMatch() || matchTrait.hasMatch()) {
            if (m_treeOn->isChecked()) {
                node = new QTreeWidgetItem(classNode, lastClassNode);
                if (m_expandOn->isChecked()) {
                    m_symbols->expandItem(node);
                }
                lastClassNode = node;
            } else {
                node = new QTreeWidgetItem(m_symbols);
            }
            if (matchClass.hasMatch()) {
                if (m_typesOn->isChecked()) {
                    nameWithTypes = matchClass.captured(3);
                    if (!matchClass.captured(1).trimmed().isEmpty() && !matchClass.captured(4).trimmed().isEmpty()) {
                        nameWithTypes +=
                            QLatin1String(" [") + matchClass.captured(1).trimmed() + QLatin1Char(',') + matchClass.captured(4).trimmed() + QLatin1Char(']');
                    } else if (!matchClass.captured(1).trimmed().isEmpty()) {
                        nameWithTypes += QLatin1String(" [") + matchClass.captured(1).trimmed() + QLatin1Char(']');
                    } else if (!matchClass.captured(4).trimmed().isEmpty()) {
                        nameWithTypes += QLatin1String(" [") + matchClass.captured(4).trimmed() + QLatin1Char(']');
                    }
                    node->setText(0, nameWithTypes);
                } else {
                    node->setText(0, matchClass.captured(3));
                }
            } else if(matchInterface.hasMatch()) {
                if (m_typesOn->isChecked()) {
                    nameWithTypes = matchInterface.captured(1) + QLatin1String(" [interface]");
                    node->setText(0, nameWithTypes);
                } else {
                    node->setText(0, matchInterface.captured(1));
                }
            } else {
                if (m_typesOn->isChecked()) {
                    nameWithTypes = matchTrait.captured(1) + QLatin1String(" [trait]");
                    node->setText(0, nameWithTypes);
                } else {
                    node->setText(0, matchTrait.captured(1));
                }
            }
            node->setIcon(0, m_icon_class);
            node->setText(1, QString::number(i, 10));
            node->setToolTip(0, nameWithTypes);
            inClass = true;
            inFunction = false;
        }

        // detect class constants
        match = constantRegExp.match(line);
        if (match.hasMatch()) {
            if (m_treeOn->isChecked()) {
                node = new QTreeWidgetItem(lastClassNode);
            } else {
                node = new QTreeWidgetItem(m_symbols);
            }
            node->setText(0, match.captured(1));
            node->setIcon(0, m_icon_typedef);
            node->setText(1, QString::number(i, 10));
        }

        // detect class variables
        if (inClass && !inFunction) {
            match = varRegExp.match(line);
            if (match.hasMatch()) {
                if (m_treeOn->isChecked()) {
                    node = new QTreeWidgetItem(lastClassNode);
                } else {
                    node = new QTreeWidgetItem(m_symbols);
                }
                node->setText(0, match.captured(4));
                node->setIcon(0, m_icon_variable);
                node->setText(1, QString::number(i, 10));
            }
        }

        // detect functions
        match = functionRegExp.match(realLine);
        if (match.hasMatch()) {
            if (m_treeOn->isChecked()) {
                if (match.captured(1).isEmpty() && match.captured(2).isEmpty()) {
                    inClass = false;
                    node = new QTreeWidgetItem(lastFunctionNode);
                } else {
                    node = new QTreeWidgetItem(lastClassNode);
                }
            } else {
                node = new QTreeWidgetItem(m_symbols);
            }

            QString functionArgs(match.captured(6));
            functionArgsList.clear();
            matchFunctionArgs = functionArgsRegExp.globalMatch(functionArgs);
            while (matchFunctionArgs.hasNext()) {
                matchFunctionArg = matchFunctionArgs.next();
                functionArgsList.append(matchFunctionArg.captured(0));
            }

            nameWithTypes = match.captured(5) + QLatin1Char('(') + functionArgsList.join(QLatin1String(", ")) + QLatin1Char(')');
            if (m_typesOn->isChecked()) {
                node->setText(0, nameWithTypes);
            } else {
                node->setText(0, match.captured(5));
            }

            node->setIcon(0, m_icon_function);
            node->setText(1, QString::number(i, 10));
            node->setToolTip(0, nameWithTypes);

            functionArgsList.clear();

            inFunction = true;
        }
    }
}
