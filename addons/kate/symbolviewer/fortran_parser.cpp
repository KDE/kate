/***************************************************************************
                      fortran_parser.cpp  -  description
                             -------------------
    begin                : jul 10 2005
    author               : 2005 Roberto Quitiliani
    email                : roby(dot)q(AT)tiscali(dot)it
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

void KatePluginSymbolViewerView::parseFortranSymbols(void)
{
 if (!mainWindow()->activeView())
   return;

 QString currline;
 QString subrStr("subroutine ");
 QString funcStr("function ");
 QString modStr("module ");

 QString stripped="";
 int i;
 int fnd,block=0,blockend=0,paro=0,parc=0;
 bool mainprog;

 QTreeWidgetItem *node = NULL;
 QTreeWidgetItem *subrNode = NULL, *funcNode = NULL, *modNode = NULL;
 QTreeWidgetItem *lastSubrNode = NULL, *lastFuncNode = NULL, *lastModNode = NULL;

 QPixmap func( ( const char** ) class_xpm );
 QPixmap subr( ( const char** ) macro_xpm );
 QPixmap mod( ( const char** ) struct_xpm );

 //It is necessary to change names
 m_macro->setText(i18n("Show Subroutines"));
 m_struct->setText(i18n("Show Modules"));
 m_func->setText(i18n("Show Functions"));

 if(m_plugin->treeOn)
  {
   funcNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Functions") ) );
   subrNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Subroutines") ) );
   modNode = new QTreeWidgetItem(m_symbols, QStringList( i18n("Modules") ) );
   funcNode->setIcon(0, QIcon(func));
   modNode->setIcon(0, QIcon(mod));
   subrNode->setIcon(0, QIcon(subr));

   if (m_plugin->expandedOn)
      {
       m_symbols->expandItem(funcNode);
       m_symbols->expandItem(subrNode);
       m_symbols->expandItem(modNode);
      }

   lastSubrNode = subrNode;
   lastFuncNode = funcNode;
   lastModNode = modNode;
   m_symbols->setRootIsDecorated(1);
  }
 else
   m_symbols->setRootIsDecorated(0);

 KTextEditor::Document *kDoc = mainWindow()->activeView()->document();

 for (i = 0; i < kDoc->lines(); i++)
   {
    currline = kDoc->line(i);
    currline = currline.trimmed();
    //currline = currline.simplified(); is this really needed ?
    //Fortran is case insensitive
    currline = currline.toLower();
    bool comment = false;
    //kdDebug(13000)<<currline<<endl;
    if(currline == "") continue;
    if(currline.at(0) == '!' || currline.at(0) == 'c') comment = true;
    //block=0;

    mainprog=false;

    if(!comment)
      {
       //Subroutines
       if(currline.startsWith(subrStr) || currline.startsWith("program "))
         {
          block=1;
          stripped="";
         }
       //Modules
        else if(currline.startsWith(modStr))
         {
          block=2;
          stripped="";
         }
       //Functions
        else if(((( currline.startsWith("real") || 
                    currline.startsWith("double") || 
                    currline.startsWith("integer") || 
                    currline.startsWith("character")) || 
                    currline.startsWith("logical") || 
                    currline.startsWith("pure") ||
                    currline.startsWith("elemental") ||
                    currline.startsWith("recursive") ||
                    currline.startsWith("type")) &&   
                    currline.indexOf(funcStr) > 0) || 
                    currline.startsWith(funcStr)                    
                )
         {
          block=3;
          stripped="";
         }

       //Subroutines
       if(block==1)
         {
          if(currline.startsWith("program "))
               mainprog=true;
          if (macro_on == true) // not really a macro, but a subroutines
            {
             stripped += currline.right(currline.length());
             stripped = stripped.simplified();
             stripped.remove('*');
             stripped.remove('+');
             stripped.remove('$');
             if(blockend==0)
               {
                fnd = stripped.indexOf(' ');
                stripped = currline.right(currline.length()-fnd-1);
               }
             stripped.remove(' ');
             fnd = stripped.indexOf('!');
             if(fnd>0)
               {
                stripped = stripped.left(fnd);
               }
             paro+=currline.count(')', Qt::CaseSensitive);
             parc+=currline.count('(', Qt::CaseSensitive);

             if((paro==parc || mainprog) && stripped.endsWith('&', Qt::CaseInsensitive)==false)
               {
                stripped.remove('&');
                if(mainprog && stripped.indexOf('(')<0 && stripped.indexOf(')')<0)
                    stripped.prepend("Main: ");
                if(stripped.indexOf('=')==-1)
                  {
                   if (m_plugin->treeOn)
                     {
                      node = new QTreeWidgetItem(subrNode, lastSubrNode);
                      lastSubrNode = node;
                     }
                   else
                      node = new QTreeWidgetItem(m_symbols);
                   node->setText(0, stripped);
                   node->setIcon(0, QIcon(subr));
                   node->setText(1, QString::number( i, 10));
                  }
                stripped="";
                block=0;
                blockend=0;
                paro=0;
                parc=0;
               }
             else
               {
                blockend=1;
               }
            }
        }

       //Modules
       else if(block==2)
        {
         if (struct_on == true) // not really a struct, but a module
           {
            stripped = currline.right(currline.length());
            stripped = stripped.simplified();
            fnd = stripped.indexOf(' ');
            stripped = currline.right(currline.length()-fnd-1);
            fnd = stripped.indexOf('!');
            if(fnd>0)
              {
               stripped = stripped.left(fnd);
              }
            if(stripped.indexOf('=')==-1)
              {
               if (m_plugin->treeOn)
                 {
                  node = new QTreeWidgetItem(modNode, lastModNode);
                  lastModNode = node;
                 }
               else
                  node = new QTreeWidgetItem(m_symbols);
               node->setText(0, stripped);
               node->setIcon(0, QIcon(mod));
               node->setText(1, QString::number( i, 10));
              }
            stripped = "";
           }
          block=0;
          blockend=0;
        }

      //Functions
      else if(block==3)
        {
         if (func_on == true)
           {
            stripped += currline.right(currline.length());
            stripped = stripped.trimmed();
            stripped.remove( "function" );
            stripped.remove('*');
            stripped.remove('+');
            stripped.remove('$');
            stripped = stripped.simplified();
            fnd = stripped.indexOf('!');
            if(fnd>0)
              {
               stripped = stripped.left(fnd);
              }
            stripped = stripped.trimmed();
            paro+=currline.count(')', Qt::CaseSensitive);
            parc+=currline.count('(', Qt::CaseSensitive);

            if(paro==parc && stripped.endsWith('&')==false)
              {
               stripped.remove('&');
              if (m_plugin->treeOn)
                {
                 node = new QTreeWidgetItem(funcNode, lastFuncNode);
                 lastFuncNode = node;
                }
              else
                 node = new QTreeWidgetItem(m_symbols);
              node->setText(0, stripped);
              node->setIcon(0, QIcon(func));
              node->setText(1, QString::number( i, 10));
              stripped = "";
              block=0;
              paro=0;
              parc=0;
             }
           blockend=0;
          }
        }
      }
    } //for i loop
}

