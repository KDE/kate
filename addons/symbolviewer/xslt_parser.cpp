/***************************************************************************
                          xslt_parser.cpp  -  description
                             -------------------
    begin                : Mar 28 2007
    author               : 2007 jiri Tyr
    email                : jiri.tyr@vslib.cz
 ***************************************************************************/
 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "plugin_katesymbolviewer.h"

void KatePluginSymbolViewerView::parseXsltSymbols(void)
{
  if (!m_mainWindow->activeView())
   return;

 m_macro->setText(i18n("Show Params"));
 m_struct->setText(i18n("Show Variables"));
 m_func->setText(i18n("Show Templates"));

 QString cl; // Current Line
 QString stripped;

 char comment = 0;
 char templ = 0;
 int i;

 QPixmap cls( ( const char** ) class_xpm );
 QPixmap sct( ( const char** ) struct_xpm );
 QPixmap mcr( ( const char** ) macro_xpm );
 QPixmap cls_int( ( const char** ) class_int_xpm );

 QTreeWidgetItem *node = nullptr;
 QTreeWidgetItem *mcrNode = nullptr, *sctNode = nullptr, *clsNode = nullptr;
 QTreeWidgetItem *lastMcrNode = nullptr, *lastSctNode = nullptr, *lastClsNode = nullptr;

 KTextEditor::Document *kv = m_mainWindow->activeView()->document();
 //kdDebug(13000)<<"Lines counted :"<<kv->numLines()<<endl;


 if(m_plugin->treeOn)
   {
    mcrNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Params") ) );
    sctNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Variables") ) );
    clsNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Templates") ) );
    mcrNode->setIcon(0, QIcon(mcr));
    sctNode->setIcon(0, QIcon(sct));
    clsNode->setIcon(0, QIcon(cls));

    if (m_plugin->expandedOn) 
      {
       m_symbols->expandItem(mcrNode);
       m_symbols->expandItem(sctNode);
       m_symbols->expandItem(clsNode);
      }

    lastMcrNode = mcrNode;
    lastSctNode = sctNode;
    lastClsNode = clsNode;

    m_symbols->setRootIsDecorated(1);
   }
 else
   {
    m_symbols->setRootIsDecorated(0);
   }

 for (i=0; i<kv->lines(); i++)
    {
     cl = kv->line(i);
     cl = cl.trimmed();

     if(cl.indexOf(QRegExp(QLatin1String("<!--"))) >= 0) { comment = 1; }
     if(cl.indexOf(QRegExp(QLatin1String("-->"))) >= 0) { comment = 0; continue; }

     if(cl.indexOf(QRegExp(QLatin1String("^</xsl:template>"))) >= 0) { templ = 0; continue; }

     if (comment==1) { continue; }
     if (templ==1) { continue; }

     if(cl.indexOf(QRegExp(QLatin1String("^<xsl:param "))) == 0 && macro_on)
       {
        QString stripped = cl.remove(QRegExp(QLatin1String("^<xsl:param +name=\"")));
        stripped = stripped.remove(QRegExp(QLatin1String("\".*")));

        if (m_plugin->treeOn)
          {
           node = new QTreeWidgetItem(mcrNode, lastMcrNode);
           lastMcrNode = node;
          }
        else node = new QTreeWidgetItem(m_symbols);
        node->setText(0, stripped);
        node->setIcon(0, QIcon(mcr));
        node->setText(1, QString::number( i, 10));
       }

     if(cl.indexOf(QRegExp(QLatin1String("^<xsl:variable "))) == 0 && struct_on)
       {
        QString stripped = cl.remove(QRegExp(QLatin1String("^<xsl:variable +name=\"")));
        stripped = stripped.remove(QRegExp(QLatin1String("\".*")));

        if (m_plugin->treeOn)
          {
           node = new QTreeWidgetItem(sctNode, lastSctNode);
           lastSctNode = node;
          }
        else node = new QTreeWidgetItem(m_symbols);
        node->setText(0, stripped);
        node->setIcon(0, QIcon(sct));
        node->setText(1, QString::number( i, 10));
       }

     if(cl.indexOf(QRegExp(QLatin1String("^<xsl:template +match="))) == 0 && func_on)
       {
        QString stripped = cl.remove(QRegExp(QLatin1String("^<xsl:template +match=\"")));
        stripped = stripped.remove(QRegExp(QLatin1String("\".*")));

        if (m_plugin->treeOn)
          {
           node = new QTreeWidgetItem(clsNode, lastClsNode);
           lastClsNode = node;
          }
        else node = new QTreeWidgetItem(m_symbols);
        node->setText(0, stripped);
        node->setIcon(0, QIcon(cls_int));
        node->setText(1, QString::number( i, 10));
       }

     if(cl.indexOf(QRegExp(QLatin1String("^<xsl:template +name="))) == 0 && func_on)
       {
        QString stripped = cl.remove(QRegExp(QLatin1String("^<xsl:template +name=\"")));
        stripped = stripped.remove(QRegExp(QLatin1String("\".*")));

        if (m_plugin->treeOn)
          {
           node = new QTreeWidgetItem(clsNode, lastClsNode);
           lastClsNode = node;
          }
        else node = new QTreeWidgetItem(m_symbols);
        node->setText(0, stripped);
        node->setIcon(0, QIcon(cls));
        node->setText(1, QString::number( i, 10));

       }

     if(cl.indexOf(QRegExp(QLatin1String("<xsl:template"))) >= 0)
       {
        templ = 1;
       }
    }
}
