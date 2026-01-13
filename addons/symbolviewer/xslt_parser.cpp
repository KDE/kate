/***************************************************************************
                          xslt_parser.cpp  -  description
                             -------------------
    begin                : Mar 28 2007
    author               : 2007 jiri Tyr
    email                : jiri.tyr@vslib.cz
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 ***************************************************************************/

#include "plugin_katesymbolviewer.h"

void KatePluginSymbolViewerView::parseXsltSymbols(void)
{
    if (!m_mainWindow->activeView()) {
        return;
    }

    m_macro->setText(i18n("Show Params"));
    m_struct->setText(i18n("Show Variables"));
    m_func->setText(i18n("Show Templates"));

    QTreeWidgetItem *node = nullptr;
    QTreeWidgetItem *mcrNode = nullptr, *sctNode = nullptr, *clsNode = nullptr;
    QTreeWidgetItem *lastMcrNode = nullptr, *lastSctNode = nullptr, *lastClsNode = nullptr;

    KTextEditor::Document *kv = m_mainWindow->activeView()->document();
    // kdDebug(13000)<<"Lines counted :"<<kv->numLines()<<endl;

    if (m_treeOn->isChecked()) {
        mcrNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Params")));
        sctNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Variables")));
        clsNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Templates")));
        mcrNode->setIcon(0, m_icon_typedef);
        sctNode->setIcon(0, m_icon_variable);
        clsNode->setIcon(0, m_icon_class);

        if (m_expandOn->isChecked()) {
            m_symbols->expandItem(mcrNode);
            m_symbols->expandItem(sctNode);
            m_symbols->expandItem(clsNode);
        }

        lastMcrNode = mcrNode;
        lastSctNode = sctNode;
        lastClsNode = clsNode;

        m_symbols->setRootIsDecorated(1);
    } else {
        m_symbols->setRootIsDecorated(0);
    }

    static const auto re4 = QRegularExpression(QStringLiteral("\".*"));

    bool is_comment = false, is_template = false;
    for (int i = 0; i < kv->lines(); i++) {
        QString cl = kv->line(i);
        cl = cl.trimmed();

        if (cl.indexOf(QLatin1String("<!--")) >= 0) {
            is_comment = true;
        }
        if (cl.indexOf(QLatin1String("-->")) >= 0) {
            is_comment = false;
            continue;
        }

        static const auto re1 = QRegularExpression(QStringLiteral("^</xsl:template>"));
        if (cl.indexOf(re1) >= 0) {
            is_template = false;
            continue;
        }

        if (is_comment || is_template) {
            continue;
        }

        static const auto re2 = QRegularExpression(QStringLiteral("^<xsl:param "));
        if (cl.indexOf(re2) == 0 && m_macro->isChecked()) {
            static const auto re3 = QRegularExpression(QStringLiteral("^<xsl:param +name=\""));
            QString stripped = cl.remove(re3);

            stripped.remove(re4);

            if (m_treeOn->isChecked()) {
                node = new QTreeWidgetItem(mcrNode, lastMcrNode);
                lastMcrNode = node;
            } else {
                node = new QTreeWidgetItem(m_symbols);
            }
            node->setText(0, stripped);
            node->setIcon(0, m_icon_typedef);
            node->setText(1, QString::number(i, 10));
        }

        static const auto re5 = QRegularExpression(QStringLiteral("^<xsl:variable "));
        if (cl.indexOf(re5) == 0 && m_struct->isChecked()) {
            static const auto re6 = QRegularExpression(QStringLiteral("^<xsl:variable +name=\""));
            QString stripped = cl.remove(re6);
            stripped.remove(re4);

            if (m_treeOn->isChecked()) {
                node = new QTreeWidgetItem(sctNode, lastSctNode);
                lastSctNode = node;
            } else {
                node = new QTreeWidgetItem(m_symbols);
            }
            node->setText(0, stripped);
            node->setIcon(0, m_icon_variable);
            node->setText(1, QString::number(i, 10));
        }

        static const auto re7 = QRegularExpression(QStringLiteral("^<xsl:template +match="));
        if (cl.indexOf(re7) == 0 && m_func->isChecked()) {
            static const auto re8 = QRegularExpression(QStringLiteral("^<xsl:template +match=\""));
            QString stripped = cl.remove(re8);
            stripped.remove(re4);

            if (m_treeOn->isChecked()) {
                node = new QTreeWidgetItem(clsNode, lastClsNode);
                lastClsNode = node;
            } else {
                node = new QTreeWidgetItem(m_symbols);
            }
            node->setText(0, stripped);
            node->setIcon(0, m_icon_context);
            node->setText(1, QString::number(i, 10));
        }

        static const auto re9 = QRegularExpression(QStringLiteral("^<xsl:template +name="));
        if (cl.indexOf(re9) == 0 && m_func->isChecked()) {
            static const auto re10 = QRegularExpression(QStringLiteral("^<xsl:template +name=\""));
            QString stripped = cl.remove(re10);
            stripped.remove(re4);

            if (m_treeOn->isChecked()) {
                node = new QTreeWidgetItem(clsNode, lastClsNode);
                lastClsNode = node;
            } else {
                node = new QTreeWidgetItem(m_symbols);
            }
            node->setText(0, stripped);
            node->setIcon(0, m_icon_class);
            node->setText(1, QString::number(i, 10));
        }

        if (cl.indexOf(QLatin1String("<xsl:template")) >= 0) {
            is_template = true;
        }
    }
}
