/***************************************************************************
                          perl_parser.cpp  -  description
                             -------------------
    begin                : Apr 2 2003
    author               : 2003 Massimo Callegari
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

void KatePluginSymbolViewerView::parsePerlSymbols(void)
{
 if (!win->activeView())
   return;

 popup->changeItem( popup->idAt(2),i18n("Show Uses"));
 popup->changeItem( popup->idAt(3),i18n("Show Pragmas"));
 popup->changeItem( popup->idAt(4),i18n("Show Subroutines"));
 QString cl; // Current Line
 QString stripped;
 char comment = 0;
 QPixmap cls( ( const char** ) class_xpm );
 QPixmap sct( ( const char** ) struct_xpm );
 QPixmap mcr( ( const char** ) macro_xpm );
 QPixmap cls_int( ( const char** ) class_int_xpm );
 QTreeWidgetItem *node = NULL;
 QTreeWidgetItem *mcrNode = NULL, *sctNode = NULL, *clsNode = NULL;
 QTreeWidgetItem *lastMcrNode = NULL, *lastSctNode = NULL, *lastClsNode = NULL;

 KTextEditor::Document *kv = win->activeView()->document();

     //kdDebug(13000)<<"Lines counted :"<<kv->numLines()<<endl;
 if(treeMode)
   {
    mcrNode = new QTreeWidgetItem(symbols, QStringList( i18n("Uses") ) );
    sctNode = new QTreeWidgetItem(symbols, QStringList( i18n("Pragmas") ) );
    clsNode = new QTreeWidgetItem(symbols, QStringList( i18n("Subroutines") ) );
    mcrNode->setIcon(0, QIcon(mcr));
    sctNode->setIcon(0, QIcon(sct));
    clsNode->setIcon(0, QIcon(cls));

    if (expanded_on)
      {
       symbols->expandItem(mcrNode);
       symbols->expandItem(sctNode);
       symbols->expandItem(clsNode);
      }
    lastMcrNode = mcrNode;
    lastSctNode = sctNode;
    lastClsNode = clsNode;
    symbols->setRootIsDecorated(1);
   }
 else
    symbols->setRootIsDecorated(0);

 for (int i=0; i<kv->lines(); i++)
   {
    cl = kv->line(i);
    cl = cl.trimmed();

    kDebug(13000)<<"Line " << i << " : "<< cl;

    if(cl == "" || cl.at(0) == '#') continue;
    if(cl.indexOf(QRegExp("^=")) >= 0) comment = 1;
    if(cl.indexOf(QRegExp("^=cut$")) >= 0)
       {
        comment = 0;
        continue;
       }
    if (comment==1)
        continue;

    if(cl.indexOf(QRegExp("^use +[A-Z]")) == 0 && macro_on)
      {
       QString stripped=cl.remove( QRegExp("^use +") );
       //stripped=stripped.replace( QRegExp(";$"), "" ); // Doesn't work ??
       stripped = stripped.left(stripped.indexOf(';'));
       if (treeMode)
         {
          node = new QTreeWidgetItem(mcrNode, lastMcrNode);
          lastMcrNode = node;
         }
       else
          node = new QTreeWidgetItem(symbols);

       node->setText(0, stripped);
       node->setIcon(0, QIcon(mcr));
       node->setText(1, QString::number( i, 10));
      }
#if 1
    if(cl.indexOf(QRegExp("^use +[a-z]")) == 0 && struct_on)
      {
       QString stripped=cl.remove( QRegExp("^use +") );
       stripped=stripped.remove( QRegExp(";$") );
       if (treeMode)
         {
          node = new QTreeWidgetItem(sctNode, lastSctNode);
          lastMcrNode = node;
         }
       else
          node = new QTreeWidgetItem(symbols);

       node->setText(0, stripped);
       node->setIcon(0, QIcon(sct));
       node->setText(1, QString::number( i, 10));
      }
#endif
#if 1
    if(cl.indexOf(QRegExp("^sub +"))==0 && func_on)
      {
       QString stripped=cl.remove( QRegExp("^sub +") );
       stripped=stripped.remove( QRegExp("[{;] *$") );
       if (treeMode)
         {
          node = new QTreeWidgetItem(clsNode, lastClsNode);
          lastClsNode = node;
         }
       else
          node = new QTreeWidgetItem(symbols);
        node->setText(0, stripped);

        if (stripped.at(0)=='_')
             node->setIcon(0, QIcon(cls_int));
        else
             node->setIcon(0, QIcon(cls));

        node->setText(1, QString::number( i, 10));
       }
#endif
   }
}



