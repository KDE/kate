/* This file is part of the KDE project
 * Copyright (C) 2009 Joseph Wenninger <jowenn@kde.org>
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "selector.h"
#include "selector.moc"
#include "jowennsnippets.h"
#include "completionmodel.h"
#include <ktexteditor/view.h>
#include <ktexteditor/templateinterface.h>
#include <kdebug.h>

namespace JoWenn {
  
  KateSnippetSelector::KateSnippetSelector(Kate::MainWindow *mainWindow,JoWenn::KateSnippetsPlugin *plugin, QWidget *parent):QWidget(parent),m_plugin(plugin),m_mainWindow(mainWindow),m_mode("_____")
  {
    setupUi(this);
    plainTextEdit->setReadOnly(true);
    connect(mainWindow,SIGNAL(viewChanged()),this,SLOT(viewChanged()));
    connect(treeView,SIGNAL(clicked(const QModelIndex&)),this,SLOT(clicked(const QModelIndex&)));
    connect(treeView,SIGNAL(doubleClicked(const QModelIndex&)),this,SLOT(doubleClicked(const QModelIndex&)));
  }
  
  KateSnippetSelector::~KateSnippetSelector()
  {
  }

  void KateSnippetSelector::viewChanged()
  {
    kDebug(13040);
    KTextEditor::View *view=m_mainWindow->activeView();
    kDebug(13040)<<view;
    QAbstractItemModel *m=treeView->model();
    
    if (view)
    {
      QString mode=view->document()->mode();
      if ((mode!=m_mode) || (treeView->model()==0))
      {
          treeView->setModel(m_plugin->modelForDocument(view->document()));
          m_mode=mode;
      }
    }
  }


  void KateSnippetSelector::clicked(const QModelIndex& current)
  {
    plainTextEdit->setPlainText(treeView->model()->data(current,KTextEditor::CodesnippetsCore::SnippetSelectorModel::FillInRole).toString());
  }

  void KateSnippetSelector::doubleClicked(const QModelIndex& current)
  {
    KTextEditor::View *view=m_mainWindow->activeView();
    KTextEditor::TemplateInterface *ti=qobject_cast<KTextEditor::TemplateInterface*>(view);
    if (ti)
      ti->insertTemplateText (view->cursorPosition(), treeView->model()->data(current,KTextEditor::CodesnippetsCore::SnippetSelectorModel::FillInRole).toString(),QMap<QString,QString>());
    view->setFocus();
  }



}

// kate: space-indent on; indent-width 2; replace-tabs on;
