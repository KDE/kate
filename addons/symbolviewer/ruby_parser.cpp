/***************************************************************************
                          ruby_parser.cpp  -  description
                             -------------------
    begin                : May 9th 2007
    author               : 2007 Massimo Callegari
    email                : massimocallegari@yahoo.it
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

void KatePluginSymbolViewerView::parseRubySymbols(void)
{
  if (!m_mainWindow->activeView())
   return;

 m_macro->setText(i18n("Show Globals"));
 m_struct->setText(i18n("Show Methods"));
 m_func->setText(i18n("Show Classes"));

 QString cl; // Current Line
 QPixmap cls( ( const char** ) class_xpm );
 QPixmap mtd( ( const char** ) method_xpm );
 QPixmap mcr( ( const char** ) macro_xpm );

 int i;
 QString name;

 QTreeWidgetItem *node = nullptr;
 QTreeWidgetItem *mtdNode = nullptr, *clsNode = nullptr;
 QTreeWidgetItem *lastMtdNode = nullptr, *lastClsNode = nullptr;

 KTextEditor::Document *kv = m_mainWindow->activeView()->document();
 //kdDebug(13000)<<"Lines counted :"<<kv->numLines()<<endl;

 if(m_plugin->treeOn)
   {
    clsNode = new QTreeWidgetItem(m_symbols);
    clsNode->setText(0, i18n("Classes"));
    clsNode->setIcon(0, QIcon(cls));
    if (m_plugin->expandedOn) m_symbols->expandItem(clsNode);
    lastClsNode = clsNode;
    mtdNode = clsNode;
    lastMtdNode = clsNode;
    m_symbols->setRootIsDecorated(1);
   }
 else
     m_symbols->setRootIsDecorated(0);

 for (i=0; i<kv->lines(); i++)
   {
    cl = kv->line(i);
    cl = cl.trimmed();

     if (cl.indexOf( QRegExp(QLatin1String("^class [a-zA-Z0-9]+[^#]")) ) >= 0)
       {
          if (func_on == true)
            {
             if (m_plugin->treeOn)
               {
                node = new QTreeWidgetItem(clsNode, lastClsNode);
                if (m_plugin->expandedOn) m_symbols->expandItem(node);
                lastClsNode = node;
                mtdNode = lastClsNode;
                lastMtdNode = lastClsNode;
               }
             else node = new QTreeWidgetItem(m_symbols);
             node->setText(0, cl.mid(6));
             node->setIcon(0, QIcon(cls));
             node->setText(1, QString::number( i, 10));
            }
       }
     if (cl.indexOf( QRegExp(QLatin1String("^def [a-zA-Z_]+[^#]")) ) >= 0 )
       {
        if (struct_on == true)
          {
           if (m_plugin->treeOn)
             {
              node = new QTreeWidgetItem(mtdNode, lastMtdNode);
              lastMtdNode = node;
             }
           else node = new QTreeWidgetItem(m_symbols);
           
           name = cl.mid(4);
           node->setToolTip(0, name);
           if (m_plugin->typesOn == false)
            {
            name = name.left(name.indexOf(QLatin1Char('(')));
            }
           node->setText(0, name);
           node->setIcon(0, QIcon(mtd));
           node->setText(1, QString::number( i, 10));
          }
       }
    }

}



