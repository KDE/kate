/***************************************************************************
                          php_parser.cpp  -  description
                             -------------------
    begin                : Apr 1st 2007
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

void KatePluginSymbolViewerView::parsePhpSymbols(void)
{
  if (!win->activeView())
   return;

 QString cl; // Current Line
 QString stripped;
 int i, j, tmpPos = 0;
 int par = 0, graph = 0, retry = 0;
 char mclass = 0, block = 0, comment = 0; // comment: 0-no comment 1-inline comment 2-multiline comment 3-string
 char func_close = 0;
 QPixmap cls( ( const char** ) class_xpm );
 QPixmap sct( ( const char** ) struct_xpm );
 QPixmap mcr( ( const char** ) macro_xpm );
 QPixmap mtd( ( const char** ) method_xpm );
 QTreeWidgetItem *node = NULL;
 QTreeWidgetItem *mcrNode = NULL, *sctNode = NULL, *clsNode = NULL, *mtdNode = NULL;
 QTreeWidgetItem *lastMcrNode = NULL, *lastSctNode = NULL, *lastClsNode = NULL, *lastMtdNode = NULL;


 KTextEditor::Document *kv = win->activeView()->document();
 //kdDebug(13000)<<"Lines counted :"<<kv->numLines()<<endl;
 if(treeMode)
   {
    mcrNode = new QTreeWidgetItem(symbols, QStringList( i18n("Macros") ) );
    sctNode = new QTreeWidgetItem(symbols, QStringList( i18n("Structures") ) );
    clsNode = new QTreeWidgetItem(symbols, QStringList( i18n("Functions") ) );
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
    mtdNode = clsNode;
    lastMtdNode = clsNode;
    symbols->setRootIsDecorated(1);
   }
 else symbols->setRootIsDecorated(0);

 for (i=0; i<kv->lines(); i++)
   {
    cl = kv->line(i);
    cl = cl.trimmed();
    func_close = 0;

    if ( (cl.length()>=2) && (cl.at(0) == '/' && cl.at(1) == '/')) continue;
    if(cl.indexOf('#') == 0) continue;
    if(cl.indexOf("/*") == 0 && (cl.indexOf("*/") == ((signed)cl.length() - 2)) && graph == 0) continue; // workaround :(
    if(cl.indexOf("/*") >= 0 && graph == 0) comment = 1;
    if(cl.indexOf("*/") >= 0 && graph == 0) comment = 0;

       if ((cl.indexOf("class") == 0 && graph == 0 && block == 0))
         {
          mclass = 1;
          for (j = 0; j < cl.length(); j++)
             {
              if(cl.at(j)=='/' && cl.at(j+1)=='/') { mclass = 2; break; }
              if(cl.at(j)=='{') { mclass = 4; break;}
              stripped += cl.at(j);
             }

          if(func_on == true)
            {
             stripped.remove("class ");
             if (treeMode)
               {
                node = new QTreeWidgetItem(clsNode, lastClsNode);
                if (expanded_on) symbols->expandItem(node); //node->setOpen(TRUE);
                lastClsNode = node;
                mtdNode = lastClsNode;
                lastMtdNode = lastClsNode;
               }
             else node = new QTreeWidgetItem(symbols);
             node->setText(0, stripped);
             node->setIcon(0, QIcon(cls));
             node->setText(1, QString::number( i, 10));
             stripped = "";
             if (mclass == 1) mclass = 3;
            }
          continue;
         }
       if (mclass == 3)
         {
          if (cl.indexOf('{') >= 0)
            {
             cl = cl.right(cl.find('{'));
             mclass = 4;
            }
         }

       if(cl.indexOf("function") == 0 && cl.at(0) != '#' && block == 0 && comment != 2)
          block = 1;

       if(block > 0 && mclass != 1 )
         {
          for (j = 0; j < cl.length(); j++)
            {
             if (cl.at(j) == '/' && (cl.at(j + 1) == '*') && comment != 3) comment = 2;
             if (cl.at(j) == '*' && (cl.at(j + 1) == '/') && comment != 3) {  comment = 0; j+=2; }
             // Handles a string. Those are freaking evilish !
             if (cl.at(j) == '"' && comment == 3) { comment = 0; }
             else if (cl.at(j) == '"' && comment == 0) comment = 3;
             if(cl.at(j)=='/' && cl.at(j+1)=='/' && comment == 0)
               { if(block == 1 && stripped.isEmpty()) block = 0; break; }
             if (comment != 2 && comment != 3)
               {
                if (block == 1 && graph == 0 )
                  {
                   if(cl.at(j) >= 0x20) stripped += cl.at(j);
                   if(cl.at(j) == '(') par++;
                   if(cl.at(j) == ')')
                     {
                      par--;
                      if(par == 0)
                        {
                         stripped = stripped.trimmed();
                         stripped.remove("static ");
                         stripped.remove("function ");
                         //kdDebug(13000)<<"Function -- Inserted : "<<stripped<<" at row : "<<i<<endl;
                         block = 2;
                         tmpPos = i;
                        }
                     }
                  } // BLOCK 1
                if(block == 2 && graph == 0)
                  {
                   if(cl.at(j)=='/' && cl.at(j+1)=='/' && comment == 0) break;
                   //if(cl.at(j)==':' || cl.at(j)==',') { block = 1; continue; }
                   if(cl.at(j)==':') { block = 1; continue; }
                   if(cl.at(j)==';')
                     {
                      stripped = "";
                      block = 0;
                      break;
                     }
                   if(cl.at(j)=='{' && cl.find(";") < 0 ||
                      cl.at(j)=='{' && cl.find('}') > (int)j)
                     {
                      stripped.replace(0x9, " ");
                      if(func_on == true)
                        {
                         if (types_on == false)
                           {
                            while (stripped.indexOf('(') >= 0)
                              stripped = stripped.left(stripped.find('('));
                            while (stripped.find("::") >= 0)
                              stripped = stripped.mid(stripped.find("::") + 2);
                            stripped = stripped.trimmed();
                            while (stripped.find(0x20) >= 0)
                              stripped = stripped.mid(stripped.find(0x20, 0) + 1);
                           }
                         //kdDebug(13000)<<"Function -- Inserted: "<<stripped<<" at row: "<<tmpPos<<" mclass: "<<(uint)mclass<<endl;
                         if (treeMode)
                           {
                            if (mclass == 4)
                              {
                               node = new QTreeWidgetItem(mtdNode, lastMtdNode);
                               lastMtdNode = node;
                              }
                            else 
                              {
                               node = new QTreeWidgetItem(clsNode, lastClsNode);
                               lastClsNode = node;
                              }
                           }
                         else node = new QTreeWidgetItem(symbols);
                         node->setText(0, stripped);
                         if (mclass == 4) node->setIcon(0, QIcon(mtd));
                         else node->setIcon(0, QIcon(cls));
                         node->setText(1, QString::number( tmpPos, 10));
                        }
                      stripped = "";
                      retry = 0;
                      block = 3;
                     }
                  } // BLOCK 2

                if (block == 3)
                  {
                   // A comment...there can be anything
                   if(cl.at(j)=='/' && cl.at(j+1)=='/' && comment == 0) break;
                   if(cl.at(j)=='{') graph++;
                   if(cl.at(j)=='}')
                     {
                      graph--;
                      if (graph == 0) { block = 0; func_close = 1; }
                     }
                  } // BLOCK 3

               } // comment != 2
             //kdDebug(13000)<<"Stripped : "<<stripped<<" at row : "<<i<<endl;
            } // End of For cycle
         } // BLOCK > 0
       if (mclass == 4 && block == 0 && func_close == 0)
         {
          if (cl.indexOf('}') >= 0) 
            {
             cl = cl.right(cl.indexOf('}'));
             mclass = 0;
            }
         }


   }

}



