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

 int paren_i, state = 0;
 QString class_name, method_name, function_name;
 QString class_sig, method_sig, function_sig;

 Q3ListViewItem *node = NULL;
 Q3ListViewItem *mcrNode = NULL, *mtdNode = NULL, *clsNode = NULL;
 Q3ListViewItem *lastMcrNode = NULL, *lastMtdNode = NULL, *lastClsNode = NULL;

 KTextEditor::Document *kv = win->activeView()->document();

 //kdDebug(13000)<<"Lines counted :"<<kv->numLines()<<endl;
 if(listMode)
   {
    clsNode = new Q3ListViewItem(symbols, symbols->lastItem(), i18n("Classes"));
    mcrNode = new Q3ListViewItem(symbols, symbols->lastItem(), i18n("Globals"));
    clsNode->setPixmap(0, (const QPixmap &)cls);
    mcrNode->setPixmap(0, (const QPixmap &)mcr);
    lastClsNode = clsNode;
    lastMcrNode = mcrNode;
    mtdNode = clsNode;
    lastMtdNode = clsNode;
    symbols->setRootIsDecorated(1);
   }
 else
     symbols->setRootIsDecorated(0);

 for (unsigned int i=0; i<kv->lines(); i++)
    {
     cl = kv->line(i);

     if(cl.indexOf( QRegExp("^class [a-zA-Z0-9_,\\s\\(\\).]+:") ) >= 0)
       {
          //KMessageBox::information(this, *line, QString("Found class on line %1").arg(line_no));
          //strip out the beginning class and the ending of a column
          class_sig = cl.trimmed ().mid (6);
          class_sig = class_sig.left (class_sig.length () - 1);
          paren_i = class_sig.indexOf ("(");
          class_name = class_sig.left (paren_i);
          if (func_on == true)
            {
             if (listMode)
               {
                node = new Q3ListViewItem(clsNode, lastClsNode, class_name);
                lastClsNode = node;
                mtdNode = lastClsNode;
                lastMtdNode = lastClsNode;
               }
             else node = new Q3ListViewItem(symbols, symbols->lastItem(), class_name);
             node->setPixmap(0, (const QPixmap &)cls);
             node->setText(1, QString::number( i, 10));
            }

          state = 1;
        }
      if(cl.indexOf( QRegExp("[\\s]+def [a-zA-Z_]+[^#]*:") ) >= 0 && state == 1)
        {
          //strip off the leading def and the ending colon
          method_sig = cl.trimmed ().mid (4);
          method_sig = method_sig.left (method_sig.indexOf (":"));
          paren_i = method_sig.indexOf ("(");
          method_name = method_sig.left (paren_i);

          if (struct_on == true)
            {
             if (listMode)
               {
                node = new Q3ListViewItem(mtdNode, lastMtdNode, method_name);
                lastMtdNode = node;
               }
             else node = new Q3ListViewItem(symbols, symbols->lastItem(), method_name);
             node->setPixmap(0, (const QPixmap &)mtd);
             node->setText(1, QString::number( i, 10));
            }
        }
      if(cl.indexOf( QRegExp("^def [a-zA-Z_]+[^#]*:") ) >= 0 )
        {
          //KMessageBox::information(symbols, cl, QString("Found global on line %1").arg(i));
          function_sig = cl.trimmed ().mid (4);
          function_sig = function_sig.left (function_sig.indexOf (":"));
          paren_i = function_sig.indexOf ("(");
          function_name = function_sig.left (paren_i);

          if (macro_on == true)
            {
             if (listMode)
               {
                node = new Q3ListViewItem(mcrNode, lastMcrNode, function_name);
                lastMcrNode = node;
               }
             else node = new Q3ListViewItem(symbols, symbols->lastItem(), function_name);
             node->setPixmap(0, (const QPixmap &)mcr);
             node->setText(1, QString::number( i, 10));
            }
          state = 0;
        }
    }

}



