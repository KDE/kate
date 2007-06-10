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
  if (!win->activeView())
   return;

 popup->changeItem( popup->idAt(2),i18n("Show Globals"));
 popup->changeItem( popup->idAt(3),i18n("Show Methods"));
 popup->changeItem( popup->idAt(4),i18n("Show Classes"));
 QString cl; // Current Line
 QPixmap cls( ( const char** ) class_xpm );
 QPixmap mtd( ( const char** ) method_xpm );
 QPixmap mcr( ( const char** ) macro_xpm );

 int i;
 QString name;

 QTreeWidgetItem *node = NULL;
 QTreeWidgetItem *mcrNode = NULL, *mtdNode = NULL, *clsNode = NULL;
 QTreeWidgetItem *lastMcrNode = NULL, *lastMtdNode = NULL, *lastClsNode = NULL;

 KTextEditor::Document *kv = win->activeView()->document();
 //kdDebug(13000)<<"Lines counted :"<<kv->numLines()<<endl;

 if(treeMode)
   {
    clsNode = new QTreeWidgetItem(symbols);
    clsNode->setText(0, i18n("Classes"));
    clsNode->setIcon(0, QIcon(cls));
    if (expanded_on) symbols->expandItem(clsNode);
    lastClsNode = clsNode;
    lastMcrNode = mcrNode;
    mtdNode = clsNode;
    lastMtdNode = clsNode;
    symbols->setRootIsDecorated(1);
   }
 else
     symbols->setRootIsDecorated(0);

 for (i=0; i<kv->lines(); i++)
   {
    cl = kv->line(i);
    cl = cl.trimmed();

     if (cl.indexOf( QRegExp("^class [a-zA-Z0-9]+[^#]") ) >= 0)
       {
        name = cl.mid(6);
          if (func_on == true)
            {
             if (treeMode)
               {
                node = new QTreeWidgetItem(clsNode, lastClsNode);
                if (expanded_on) symbols->expandItem(node);
                lastClsNode = node;
                mtdNode = lastClsNode;
                lastMtdNode = lastClsNode;
               }
             else node = new QTreeWidgetItem(symbols);
             node->setText(0, name);
             node->setIcon(0, QIcon(cls));
             node->setText(1, QString::number( i, 10));
            }
       }
     if (cl.indexOf( QRegExp("^def [a-zA-Z_]+[^#]") ) >= 0 )
       {
        name = cl.mid(4);
        if (types_on == false)
          {
           name = name.left(name.indexOf('('));
          }
        if (struct_on == true)
          {
           if (treeMode)
             {
              node = new QTreeWidgetItem(mtdNode, lastMtdNode);
              lastMtdNode = node;
             }
           else node = new QTreeWidgetItem(symbols);
           node->setText(0, name);
           node->setIcon(0, QIcon(mtd));
           node->setText(1, QString::number( i, 10));
          }
       }
    }

}



