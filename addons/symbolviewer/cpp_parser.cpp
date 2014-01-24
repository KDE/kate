/***************************************************************************
                          cpp_parser.cpp  -  description
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

void KatePluginSymbolViewerView::parseCppSymbols(void)
{
  if (!mainWindow()->activeView())
   return;

 QString cl; // Current Line
 QString stripped;
 int i, j, tmpPos = 0;
 int par = 0, graph = 0/*, retry = 0*/;
 char mclass = 0, block = 0, comment = 0; // comment: 0-no comment 1-inline comment 2-multiline comment 3-string
 char macro = 0/*, macro_pos = 0*/, func_close = 0;
 bool structure = false;
 QPixmap cls( ( const char** ) class_xpm );
 QPixmap sct( ( const char** ) struct_xpm );
 QPixmap mcr( ( const char** ) macro_xpm );
 QPixmap mtd( ( const char** ) method_xpm );

 //It is necessary to change names to defaults
 m_macro->setText(i18n("Show Macros"));
 m_struct->setText(i18n("Show Structures"));
 m_func->setText(i18n("Show Functions"));

 QTreeWidgetItem *node = NULL;
 QTreeWidgetItem *mcrNode = NULL, *sctNode = NULL, *clsNode = NULL, *mtdNode = NULL;
 QTreeWidgetItem *lastMcrNode = NULL, *lastSctNode = NULL, *lastClsNode = NULL, *lastMtdNode = NULL;

 KTextEditor::Document *kv = mainWindow()->activeView()->document();

 //qDebug(13000)<<"Lines counted :"<<kv->lines();
 if(m_plugin->treeOn)
   {
    mcrNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Macros") ) );
    sctNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Structures") ) );
    clsNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Functions") ) );
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
    mtdNode = clsNode;
    lastMtdNode = clsNode;
    m_symbols->setRootIsDecorated(1);
   }
 else m_symbols->setRootIsDecorated(0);

 for (i=0; i<kv->lines(); i++)
   {
    //qDebug(13000)<<"Current line :"<<i;
    cl = kv->line(i);
    cl = cl.trimmed();
    func_close = 0;
    if ( (cl.length()>=2) && (cl.at(0) == '/' && cl.at(1) == '/')) continue;
    if(cl.indexOf("/*") == 0 && (cl.indexOf("*/") == ((signed)cl.length() - 2)) && graph == 0) continue; // workaround :(
    if(cl.indexOf("/*") >= 0 && graph == 0) comment = 1;
    if(cl.indexOf("*/") >= 0 && graph == 0) comment = 0;
    if(cl.indexOf('#') >= 0 && graph == 0 ) macro = 1;
    if (comment != 1)
      {
       /* *********************** MACRO PARSING *****************************/
       if(macro == 1)
         {
          //macro_pos = cl.indexOf('#');
          for (j = 0; j < cl.length(); j++)
             {
              if ( ((j+1) <cl.length()) &&  (cl.at(j)=='/' && cl.at(j+1)=='/')) { macro = 4; break; }
              if(  cl.indexOf("define") == j &&
                 !(cl.indexOf("defined") == j))
                    {
                     macro = 2;
                     j += 6; // skip the word "define"
                    }
              if(macro == 2 && j<cl.length() &&cl.at(j) != ' ') macro = 3;
              if(macro == 3)
                {
                 if (cl.at(j) >= 0x20) stripped += cl.at(j);
                 if (cl.at(j) == ' ' || j == cl.length() - 1)
                         macro = 4;
                }
              //qDebug(13000)<<"Macro -- Stripped : "<<stripped<<" macro = "<<macro;
             }
           // I didn't find a valid macro e.g. include
           if(j == cl.length() && macro == 1) macro = 0;
           if(macro == 4)
             {
              //stripped.replace(0x9, " ");
              stripped = stripped.trimmed();
              if (macro_on == true)
                 {
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
              macro = 0;
              //macro_pos = 0;
              stripped = "";
              //qDebug(13000)<<"Macro -- Inserted : "<<stripped<<" at row : "<<i;
              if (cl.at(cl.length() - 1) == '\\') macro = 5; // continue in rows below
              continue;
             }
          }
       if (macro == 5)
          {
           if (cl.length() == 0 || cl.at(cl.length() - 1) != '\\')
               macro = 0;
           continue;
          }

       /* ******************************************************************** */

       if ((cl.indexOf("class") >= 0 && graph == 0 && block == 0))
         {
          mclass = 1;
          for (j = 0; j < cl.length(); j++)
             {
              if(((j+1) < cl.length()) && (cl.at(j)=='/' && cl.at(j+1)=='/')) { mclass = 2; break; }
              if(cl.at(j)=='{') { mclass = 4; break;}
              stripped += cl.at(j);
             }
          if(func_on == true)
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
             cl = cl.mid(cl.indexOf('{'));
             mclass = 4;
            }
         }

       if(cl.indexOf('(') >= 0 && cl.at(0) != '#' && block == 0 && comment != 2)
          { structure = false; block = 1; }
       if((cl.indexOf("typedef") >= 0 || cl.indexOf("struct") >= 0) &&
          graph == 0 && block == 0)
         { structure = true; block = 2; stripped = ""; }
       //if(cl.indexOf(';') >= 0 && graph == 0)
       //    block = 0;
       if(block > 0 && mclass != 1 )
         {
          for (j = 0; j < cl.length(); j++)
            {
             if ( ((j+1) < cl.length()) && (cl.at(j) == '/' && (cl.at(j + 1) == '*') && comment != 3)) comment = 2;
             if ( ((j+1) < cl.length()) && (cl.at(j) == '*' && (cl.at(j + 1) == '/') && comment != 3) )
                   {  comment = 0; j+=2; if (j>=cl.length()) break;}
             // Handles a string. Those are freaking evilish !
             if (cl.at(j) == '"' && comment == 3) { comment = 0; j++; if (j>=cl.length()) break;}
             else if (cl.at(j) == '"' && comment == 0) comment = 3;
             if ( ((j+1) <cl.length()) &&(cl.at(j)=='/' && cl.at(j+1)=='/') && comment == 0 )
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
                         //qDebug(13000)<<"Function -- Inserted : "<<stripped<<" at row : "<<i;
                         block = 2;
                         tmpPos = i;
                        }
                     }
                  } // BLOCK 1
                if(block == 2 && graph == 0)
                  {
                   if ( ((j+1)<cl.length()) && (cl.at(j)=='/' && cl.at(j+1)=='/') && comment == 0) break;
                   //if(cl.at(j)==':' || cl.at(j)==',') { block = 1; continue; }
                   if(cl.at(j)==':') { block = 1; continue; }
                   if(cl.at(j)==';')
                     {
                      stripped = "";
                      block = 0;
                      structure = false;
                      break;
                     }

                   if((cl.at(j)=='{' && structure == false && cl.indexOf(';') < 0) ||
                      (cl.at(j)=='{' && structure == false && cl.indexOf('}') > (int)j))
                     {
                      stripped.replace(0x9, " ");
                      if(func_on == true)
                        {
                         if (m_plugin->typesOn == false)
                           {
                            while (stripped.indexOf('(') >= 0)
                              stripped = stripped.left(stripped.indexOf('('));
                            while (stripped.indexOf("::") >= 0)
                              stripped = stripped.mid(stripped.indexOf("::") + 2);
                            stripped = stripped.trimmed();
                            while (stripped.indexOf(0x20) >= 0)
                              stripped = stripped.mid(stripped.indexOf(0x20, 0) + 1);
                            while ( 
                                (stripped.length()>0) &&
                                  ( 
                                    (stripped.at(0)=='*') ||
                                    (stripped.at(0)=='&')
                                  )
                              ) stripped=stripped.right(stripped.length()-1);
                           }
                         if (m_plugin->treeOn)
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
                         else
                             node = new QTreeWidgetItem(m_symbols);
                         node->setText(0, stripped);
                         if (mclass == 4) node->setIcon(0, QIcon(mtd));
                         else node->setIcon(0, QIcon(cls));
                         node->setText(1, QString::number( tmpPos, 10));
                        }
                      stripped = "";
                      //retry = 0;
                      block = 3;
                     }
                   if(cl.at(j)=='{' && structure == true)
                     {
                      block = 3;
                      tmpPos = i;
                     }
                   if(cl.at(j)=='(' && structure == true)
                     {
                      //retry = 1;
                      block = 0;
                      j = 0;
                      //qDebug(13000)<<"Restart from the beginning of line...";
                      stripped = "";
                      break; // Avoid an infinite loop :(
                     }
                   if(structure == true && cl.at(j) >= 0x20) stripped += cl.at(j);
                  } // BLOCK 2

                if (block == 3)
                  {
                   // A comment...there can be anything
                   if( ((j+1)<cl.length()) && (cl.at(j)=='/' && cl.at(j+1)=='/') && comment == 0 ) break;
                   if(cl.at(j)=='{') graph++;
                   if(cl.at(j)=='}')
                     {
                      graph--;
                      if (graph == 0 && structure == false)  { block = 0; func_close = 1; }
                      if (graph == 0 && structure == true) block = 4;
                     }
                  } // BLOCK 3

                if (block == 4)
                  {
                   if(cl.at(j) == ';')
                     {
                      //stripped.replace(0x9, " ");
                      stripped.remove('{');
                      stripped.replace('}', " ");
                      if(struct_on == true)
                        {
                         if (m_plugin->treeOn)
                           {
                            node = new QTreeWidgetItem(sctNode, lastSctNode);
                            lastSctNode = node;
                           }
                         else node = new QTreeWidgetItem(m_symbols);
                         node->setText(0, stripped);
                         node->setIcon(0, QIcon(sct));
                         node->setText(1, QString::number( tmpPos, 10));
                        }
                      //qDebug(13000)<<"Structure -- Inserted : "<<stripped<<" at row : "<<i;
                      stripped = "";
                      block = 0;
                      structure = false;
                      //break;
                      continue;
                     }
                   if (cl.at(j) >= 0x20) stripped += cl.at(j);
                  } // BLOCK 4
               } // comment != 2
             //qDebug(13000)<<"Stripped : "<<stripped<<" at row : "<<i;
            } // End of For cycle
         } // BLOCK > 0
       if (mclass == 4 && block == 0 && func_close == 0)
         {
          if (cl.indexOf('}') >= 0)
            {
             cl = cl.mid(cl.indexOf('}'));
             mclass = 0;
            }
         }
      } // Comment != 1
   } // for kv->numlines

 //for (i= 0; i < (m_symbols->itemIndex(node) + 1); i++)
 //    qDebug(13000)<<"Symbol row :"<<positions.at(i);
}


