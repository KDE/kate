/***************************************************************************
                      bash_parser.cpp  -  description
                             -------------------
    begin                : dec 12 2008
    author               : Daniel Dumitrache
    email                : daniel.dumitrache@gmail.com
 ***************************************************************************/
 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "plugin_katesymbolviewer.h"

void KatePluginSymbolViewerView::parseBashSymbols(void)
{
       if (!win->activeView())
               return;

       QString currline;
       QString funcStr("function ");

       int i;
       bool mainprog;

       QTreeWidgetItem *node = NULL;
       QTreeWidgetItem *funcNode = NULL;
       QTreeWidgetItem *lastFuncNode = NULL;

       QPixmap func( ( const char** ) class_xpm );

       //It is necessary to change names
       popup->changeItem( popup->idAt(4),i18n("Show Functions"));

       if(treeMode)
       {
               funcNode = new QTreeWidgetItem(symbols, QStringList(i18n("Functions") ) );
               funcNode->setIcon(0, QIcon(func));

               if (expanded_on)
               {
                       symbols->expandItem(funcNode);
               }

               lastFuncNode = funcNode;

               symbols->setRootIsDecorated(1);
       }
       else
               symbols->setRootIsDecorated(0);

       KTextEditor::Document *kDoc = win->activeView()->document();

       for (i = 0; i < kDoc->lines(); i++)
       {
               currline = kDoc->line(i);
               currline = currline.trimmed();
               currline = currline.simplified();

               bool comment = false;
               //kDebug(13000)<<currline<<endl;
               if(currline == "") continue;
               if(currline.at(0) == '#') comment = true;

               mainprog=false;
               if(!comment && func_on)
               {
                       QString funcName;

                       // skip line if no function defined
                       // note: function name must match regex: [a-zA-Z0-9-_]+
                       if(!currline.contains(QRegExp("^(function )*[a-zA-Z0-9-_]+ *\\( *\\)"))
                               && !currline.contains(QRegExp("^function [a-zA-Z0-9-_]+")))
                               continue;

                       // strip everything unneeded and get the function's name
                       currline.remove(QRegExp("^(function )*"));
                       funcName = currline.split(QRegExp("((\\( *\\))|[^a-zA-Z0-9-_])"))[0].simplified();
                       if(!funcName.size())
                               continue;
                       funcName.append("()");

                       if (treeMode)
                       {
                               node = new QTreeWidgetItem(funcNode, lastFuncNode);
                               lastFuncNode = node;
                       }
                       else
                               node = new QTreeWidgetItem(symbols);

                       node->setText(0, funcName);
                       node->setIcon(0, QIcon(func));
                       node->setText(1, QString::number( i, 10));
               }
       } //for i loop
}