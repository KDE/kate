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
 if (!mainWindow()->activeView())
   return;

 m_macro->setText(i18n("Show Uses"));
 m_struct->setText(i18n("Show Pragmas"));
 m_func->setText(i18n("Show Subroutines"));
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

 KTextEditor::Document *kv = mainWindow()->activeView()->document();

     //kdDebug(13000)<<"Lines counted :"<<kv->numLines()<<endl;
 if(m_plugin->treeOn)
   {
    mcrNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Uses") ) );
    sctNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Pragmas") ) );
    clsNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Subroutines") ) );
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
    m_symbols->setRootIsDecorated(0);

 for (int i=0; i<kv->lines(); i++)
   {
    cl = kv->line(i);
    qDebug(13000)<<"Line " << i << " : "<< cl;

    if(cl.isEmpty() || cl.at(0) == '#') continue;
    if(cl.indexOf(QRegExp("^=[a-zA-Z]")) >= 0) comment = 1;
    if(cl.indexOf(QRegExp("^=cut$")) >= 0)
       {
        comment = 0;
        continue;
       }
    if (comment==1)
        continue;
    
    cl = cl.trimmed();
    qDebug(13000)<<"Trimmed line " << i << " : "<< cl;

    if(cl.indexOf(QRegExp("^use +[A-Z]")) == 0 && macro_on)
      {
       QString stripped=cl.remove( QRegExp("^use +") );
       //stripped=stripped.replace( QRegExp(";$"), "" ); // Doesn't work ??
       stripped = stripped.left(stripped.indexOf(';'));
       if (m_plugin->treeOn)
         {
          node = new QTreeWidgetItem(mcrNode, lastMcrNode);
          lastMcrNode = node;
         }
       else
          node = new QTreeWidgetItem(m_symbols);

       node->setText(0, stripped);
       node->setIcon(0, QIcon(mcr));
       node->setText(1, QString::number( i, 10));
      }
#if 1
    if(cl.indexOf(QRegExp("^use +[a-z]")) == 0 && struct_on)
      {
       QString stripped=cl.remove( QRegExp("^use +") );
       stripped=stripped.remove( QRegExp(";$") );
       if (m_plugin->treeOn)
         {
          node = new QTreeWidgetItem(sctNode, lastSctNode);
          lastMcrNode = node;
         }
       else
          node = new QTreeWidgetItem(m_symbols);

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
       if (m_plugin->treeOn)
         {
          node = new QTreeWidgetItem(clsNode, lastClsNode);
          lastClsNode = node;
         }
       else
          node = new QTreeWidgetItem(m_symbols);
        node->setText(0, stripped);

        if (!stripped.isEmpty() && stripped.at(0)=='_')
             node->setIcon(0, QIcon(cls_int));
        else
             node->setIcon(0, QIcon(cls));

        node->setText(1, QString::number( i, 10));
       }
#endif
   }
}



