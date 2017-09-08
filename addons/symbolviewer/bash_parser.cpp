/***************************************************************************
                      bash_parser.cpp  -  description
                             -------------------
    begin                : dec 12 2008
    author               : Daniel Dumitrache
    email                : daniel.dumitrache@gmail.com
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

void KatePluginSymbolViewerView::parseBashSymbols(void)
{
       if (!m_mainWindow->activeView())
               return;

       QString currline;
       QString funcStr(QLatin1String("function "));

       int i;
       //bool mainprog;

       QTreeWidgetItem *node = nullptr;
       QTreeWidgetItem *funcNode = nullptr;
       QTreeWidgetItem *lastFuncNode = nullptr;

       QPixmap func( ( const char** ) class_xpm );

       //It is necessary to change names
       m_func->setText(i18n("Show Functions"));

       if(m_plugin->treeOn)
       {
               funcNode = new QTreeWidgetItem(m_symbols, QStringList(i18n("Functions") ) );
               funcNode->setIcon(0, QIcon(func));

               if (m_plugin->expandedOn)
               {
                       m_symbols->expandItem(funcNode);
               }

               lastFuncNode = funcNode;

               m_symbols->setRootIsDecorated(1);
       }
       else
               m_symbols->setRootIsDecorated(0);

       KTextEditor::Document *kDoc = m_mainWindow->activeView()->document();

       for (i = 0; i < kDoc->lines(); i++)
       {
               currline = kDoc->line(i);
               currline = currline.trimmed();
               currline = currline.simplified();

               bool comment = false;
               //qDebug(13000)<<currline<<endl;
               if(currline.isEmpty()) continue;
               if(currline.at(0) == QLatin1Char('#')) comment = true;

               //mainprog=false;
               if(!comment && func_on)
               {
                       QString funcName;

                       // skip line if no function defined
                       // note: function name must match regex: [a-zA-Z0-9-_]+
                       if(!currline.contains(QRegExp(QLatin1String("^(function )*[a-zA-Z0-9-_]+ *\\( *\\)")))
                               && !currline.contains(QRegExp(QLatin1String("^function [a-zA-Z0-9-_]+"))))
                               continue;

                       // strip everything unneeded and get the function's name
                       currline.remove(QRegExp(QLatin1String("^(function )*")));
                       funcName = currline.split(QRegExp(QLatin1String("((\\( *\\))|[^a-zA-Z0-9-_])")))[0].simplified();
                       if(!funcName.size())
                               continue;
                       funcName.append(QLatin1String("()"));

                       if (m_plugin->treeOn)
                       {
                               node = new QTreeWidgetItem(funcNode, lastFuncNode);
                               lastFuncNode = node;
                       }
                       else
                               node = new QTreeWidgetItem(m_symbols);

                       node->setText(0, funcName);
                       node->setIcon(0, QIcon(func));
                       node->setText(1, QString::number( i, 10));
               }
       } //for i loop
}
