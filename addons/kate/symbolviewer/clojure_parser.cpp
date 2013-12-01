/***************************************************************************
                      clojure_parser.cpp  -  Description
                             -------------------
    Begin                : 2013-12-01
    Author               : Ruediger Gad
    Email                : r.c.g@gmx.de
    Based on bash_parser.cpp
 ***************************************************************************/
 /***************************************************************************
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************/

#include "plugin_katesymbolviewer.h"

void KatePluginSymbolViewerView::parseClojureSymbols(void)
{
       if (!mainWindow()->activeView())
               return;

       QString currline;

       int i;

       QTreeWidgetItem *node = NULL;
       QTreeWidgetItem *funcNode = NULL;
       QTreeWidgetItem *lastFuncNode = NULL;

       QPixmap pixFn( ( const char** ) method_xpm );
       QPixmap pixMacro( ( const char** ) macro_xpm );
       QPixmap pixMulti( ( const char** ) class_int_xpm );
       QPixmap pixTest( ( const char** ) class_xpm );
       QPixmap pixVar( ( const char** ) struct_xpm );

       //It is necessary to change names
       m_popup->changeItem( m_popup->idAt(4),i18n("Show Clojure Functions"));

       if (treeMode)
       {
               funcNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Functions") ) );
               funcNode->setIcon(0, QIcon(pixTest));

               if (m_plugin->expanded_on)
               {
                       m_symbols->expandItem(funcNode);
               }

               lastFuncNode = funcNode;

               m_symbols->setRootIsDecorated(1);
       }
       else
               m_symbols->setRootIsDecorated(0);

       KTextEditor::Document *kDoc = mainWindow()->activeView()->document();

       for (i = 0; i < kDoc->lines(); i++)
       {
               currline = kDoc->line(i);
               currline = currline.trimmed();
               currline = currline.simplified();

               bool comment = false;
               if (currline == "") continue;
               if (currline.contains(QRegExp("^\\s*;"))) comment = true;

               if (!comment && func_on)
               {
                       QString funcName;

                       // skip line if no function defined
                       // note: function name must match regex: [a-zA-Z0-9-_]+
                       if (!currline.contains(QRegExp("^\\s*\\(def")))
                               continue;

                       // strip everything unneeded and get the function's name
                       QString tmpLine = QString(currline).remove(QRegExp("^\\s*\\(def\\S*\\s*"));
                       funcName = tmpLine.split(QRegExp("((\\( *\\))|[^a-zA-Z0-9-_])"))[0].simplified();
                       if(!funcName.size())
                               continue;

                       if (treeMode)
                       {
                               node = new QTreeWidgetItem(funcNode, lastFuncNode);
                               lastFuncNode = node;
                       }
                       else
                               node = new QTreeWidgetItem(m_symbols);

                       if (currline.contains(QRegExp("defn")))
                           node->setIcon(0, QIcon(pixFn));
                       else if (currline.contains(QRegExp("defmacro")))
                           node->setIcon(0, QIcon(pixMacro));
                       else if (currline.contains(QRegExp("deftest")))
                           node->setIcon(0, QIcon(pixTest));
                       else if (currline.contains(QRegExp("defmulti"))
                                 || currline.contains(QRegExp("defmethod")))
                           node->setIcon(0, QIcon(pixMulti));
                       else
                           node->setIcon(0, QIcon(pixVar));

                       node->setText(0, funcName);
                       node->setText(1, QString::number( i, 10));
               }
       } //for i loop
}
