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

void KatePluginSymbolViewerView::parsePythonSymbols(void)
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

 int in_class = 0, state = 0, j;
 QString name;

 QTreeWidgetItem *node = NULL;
 QTreeWidgetItem *mcrNode = NULL, *mtdNode = NULL, *clsNode = NULL;
 QTreeWidgetItem *lastMcrNode = NULL, *lastMtdNode = NULL, *lastClsNode = NULL;

 KTextEditor::Document *kv = win->activeView()->document();

 //kdDebug(13000)<<"Lines counted :"<<kv->numLines()<<endl;
 if(treeMode)
   {
    clsNode = new QTreeWidgetItem(symbols, QStringList( i18n("Classes") ) );
    mcrNode = new QTreeWidgetItem(symbols, QStringList( i18n("Globals") ) );
    mcrNode->setIcon(0, QIcon(mcr));
    clsNode->setIcon(0, QIcon(cls));

    if (expanded_on)
      {
       symbols->expandItem(mcrNode);
       symbols->expandItem(clsNode);
      }
    lastClsNode = clsNode;
    lastMcrNode = mcrNode;
    mtdNode = clsNode;
    lastMtdNode = clsNode;
    symbols->setRootIsDecorated(1);
   }
 else
     symbols->setRootIsDecorated(0);

 for (int i=0; i<kv->lines(); i++)
    {
     cl = kv->line(i);

    if(cl.indexOf( QRegExp("^class [a-zA-Z0-9_,\\s\\(\\).]+:") ) >= 0) in_class = 1;

     //if(cl.find( QRegExp("[\\s]+def [a-zA-Z_]+[^#]*:") ) >= 0) in_class = 2;
     if(cl.indexOf( QRegExp("^def [a-zA-Z_]+[^#]*:") ) >= 0 ) in_class = 0;

     if (cl.indexOf("def ") >= 0 || (cl.indexOf("class ") >= 0 && in_class == 1))
       {
        if (cl.indexOf("def ") >= 0 && in_class == 1) in_class = 2;
        state = 1;
        if (cl.indexOf(":") >= 0) state = 3; // found in the same line. Done
        else if (cl.indexOf("(") >= 0) state = 2;

        if (state == 2 || state == 3) name = cl.left (cl.indexOf ("("));
       }

     if (state > 0 && state < 3)
       {
        for (j = 0; j < cl.length(); j++)
           {
            if (cl.at(j) == '(') state = 2;
            else if (cl.at(j) == ':') { state = 3; break; }

            if (state == 1) name += cl.at(j);
           }
       }
     if (state == 3)
       {
        //kDebug(13000)<<"Function -- Inserted : "<<name<<" at row : "<<i;
        if (in_class == 1) //strip off the word "class "
            name = name.trimmed ().mid (6);
        else //strip off the word "def "
            name = name.trimmed ().mid (4);

          if (func_on == true && in_class == 1)
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

         if (struct_on == true && in_class == 2)
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

          if (macro_on == true && in_class == 0)
            {
             if (treeMode)
               {
                node = new QTreeWidgetItem(mcrNode, lastMcrNode);
                lastMcrNode = node;
               }
             else node = new QTreeWidgetItem(symbols);

             node->setText(0, name);
             node->setIcon(0, QIcon(mcr));
             node->setText(1, QString::number( i, 10));
            }

         state = 0;
         name = "";
        }
    }

}



