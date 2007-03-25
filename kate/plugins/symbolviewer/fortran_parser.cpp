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
 if (!win->activeView())
   return;

 QString currline;
 QString subrStr("subroutine ");
 QString funcStr("function ");
 QString modStr("module ");

 QString stripped="";
 uint i;
 int fnd,block=0,blockend=0,paro=0,parc=0;
 bool mainprog;

 Q3ListViewItem *node = NULL;
 Q3ListViewItem *subrNode = NULL, *funcNode = NULL, *modNode = NULL;
 Q3ListViewItem *lastSubrNode = NULL, *lastFuncNode = NULL, *lastModNode = NULL;

 QPixmap func( ( const char** ) class_xpm );
 QPixmap subr( ( const char** ) macro_xpm );
 QPixmap mod( ( const char** ) struct_xpm );

 //It is necessary to change names
 popup->changeItem( popup->idAt(2),i18n("Show Subroutines"));
 popup->changeItem( popup->idAt(3),i18n("Show Modules"));
 popup->changeItem( popup->idAt(4),i18n("Show Functions"));


 if(treeMode)
  {
   funcNode = new Q3ListViewItem(symbols, symbols->lastItem(), i18n("Functions"));
   subrNode = new Q3ListViewItem(symbols, symbols->lastItem(), i18n("Subroutines"));
   modNode = new Q3ListViewItem(symbols, symbols->lastItem(), i18n("Modules"));
   funcNode->setPixmap(0, (const QPixmap &)func);
   modNode->setPixmap(0, (const QPixmap &)mod);
   subrNode->setPixmap(0, (const QPixmap &)subr);

   if (expanded_on)
      {
       funcNode->setOpen(TRUE);
       subrNode->setOpen(TRUE);
       modNode->setOpen(TRUE);
      }

   lastSubrNode = subrNode;
   lastFuncNode = funcNode;
   lastModNode = modNode;
   symbols->setRootIsDecorated(1);
  }
 else
   symbols->setRootIsDecorated(0);

 KTextEditor::Document *kDoc = win->activeView()->document();

 for (i = 0; i<kDoc->lines(); i++)
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
       else if(((currline.startsWith("real") || currline.startsWith("double") || currline.startsWith("integer") || currline.startsWith("character")) || currline.startsWith("logical")) && currline.indexOf(funcStr) > 0)
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
             stripped.replace("*","");
             stripped.replace("+","");
             stripped.replace("$","");
             if(blockend==0)
               {
                fnd = stripped.indexOf(' ');
                stripped = currline.right(currline.length()-fnd-1);
               }
             stripped.replace(" ","");
             fnd = stripped.indexOf("!");
             if(fnd>0)
               {
                stripped = stripped.left(fnd);
               }
             paro+=currline.count(')', Qt::CaseSensitive);
             parc+=currline.count('(', Qt::CaseSensitive);

             if((paro==parc || mainprog) && stripped.endsWith("&", Qt::CaseInsensitive)==FALSE)
               {
                stripped.replace("&","");
                if(mainprog && stripped.indexOf('(')<0 && stripped.indexOf(')')<0)
                    stripped.prepend("Main: ");
                if(stripped.indexOf("=")==-1)
                  {
                   if (treeMode)
                     {
                      node = new Q3ListViewItem(subrNode, lastSubrNode, stripped);
                      lastSubrNode = node;
                     }
                   else
                      node = new Q3ListViewItem(symbols, symbols->lastItem(), stripped);

                   node->setPixmap(0, (const QPixmap &)subr);
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
               if (treeMode)
                 {
                  node = new Q3ListViewItem(modNode, lastModNode, stripped);
                  lastModNode = node;
                 }
               else
                  node = new Q3ListViewItem(symbols, symbols->lastItem(), stripped);
               node->setPixmap(0, (const QPixmap &)mod);
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
            stripped.replace( "function", "" );
            stripped.replace("*","");
            stripped.replace("+","");
            stripped.replace("$","");
            stripped = stripped.simplified();
            fnd = stripped.indexOf('!');
            if(fnd>0)
              {
               stripped = stripped.left(fnd);
              }
            stripped = stripped.trimmed();
            paro+=currline.count(')', Qt::CaseSensitive);
            parc+=currline.count('(', Qt::CaseSensitive);

            if(paro==parc && stripped.endsWith("&",FALSE)==FALSE)
              {
               stripped.replace("&","");
              if (treeMode)
                {
                 node = new Q3ListViewItem(funcNode, lastFuncNode, stripped);
                 lastFuncNode = node;
                }
              else
                 node = new Q3ListViewItem(symbols, symbols->lastItem(), stripped);
              node->setPixmap(0, (const QPixmap &)func);
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

