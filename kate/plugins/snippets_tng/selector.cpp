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
#include "repository.h"
#include <ktexteditor/view.h>
#include <ktexteditor/templateinterface.h>
#include <ktexteditor/highlightinterface.h>
#include <kate/pluginconfigpageinterface.h>
#include <kdebug.h>
#include <qmenu.h>

#define ON_THE_GO_TEMPLATESTR "%1 On-The-Go"

namespace JoWenn {
  
  KateSnippetSelector::KateSnippetSelector(Kate::MainWindow *mainWindow,JoWenn::KateSnippetsPlugin *plugin, QWidget *parent):QWidget(parent),m_plugin(plugin),m_mainWindow(mainWindow),m_mode("_____")
  {
    setupUi(this);
    plainTextEdit->setReadOnly(true);
    addSnippetToButton->setIcon(KIcon("snippetadd"));
    addSnippetToButton->setToolTip(i18n("Add current text selection to a snippet file"));
    editSnippet->setIcon(KIcon("snippetedit"));
    editSnippet->setToolTip(i18n("Modify the current snippet"));
    newRepoButton->setIcon(KIcon("repoadd"));
    newRepoButton->setToolTip(i18n("Create a new repository file"));
    showRepoManagerButton->setIcon(KIcon("repomanage"));
    showRepoManagerButton->setToolTip(i18n("Manage the snippet repository"));
    connect(mainWindow,SIGNAL(viewChanged()),this,SLOT(viewChanged()));
    connect(plugin,SIGNAL(typeHasChanged(KTextEditor::Document*)),this,SLOT(typeChanged(KTextEditor::Document*)));
    connect(treeView,SIGNAL(clicked(const QModelIndex&)),this,SLOT(clicked(const QModelIndex&)));
    connect(treeView,SIGNAL(doubleClicked(const QModelIndex&)),this,SLOT(doubleClicked(const QModelIndex&)));
    connect(hideShowBtn,SIGNAL(clicked()),this,SLOT(showHideSnippetText()));
    connect(showRepoManagerButton,SIGNAL(clicked()),this,SLOT(showRepoManager()));
    m_addSnippetToPopup = new QMenu(this);
    addSnippetToButton->setDelayedMenu(m_addSnippetToPopup);
    connect(m_addSnippetToPopup,SIGNAL(aboutToShow()),this,SLOT(addSnippetToPopupAboutToShow()));
  }
  
  KateSnippetSelector::~KateSnippetSelector()
  {
  }

  void KateSnippetSelector::showHideSnippetText()
  {
    if (plainTextEdit->isHidden())
    {
      plainTextEdit->show();
      hideShowBtn->setArrowType(Qt::DownArrow);
    }
    else
    {
      plainTextEdit->hide();
      hideShowBtn->setArrowType(Qt::UpArrow);
    }
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

  void KateSnippetSelector::typeChanged(KTextEditor::Document* document) {
    KTextEditor::View *view=m_mainWindow->activeView();
    kDebug(13040);
    if (!view) return;    
    if (view->document()==document)
    {
      kDebug(13040)<<"calling view changed";
      viewChanged();
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

  void KateSnippetSelector::showRepoManager()
  {
    m_mainWindow->showPluginConfigPage(
      dynamic_cast<Kate::PluginConfigPageInterface*> (m_plugin),
      0);
  }

  void KateSnippetSelector::addSnippetToPopupAboutToShow()
  {
      m_addSnippetToPopup->clear();
      if (!treeView->model()) return;
      QAbstractItemModel *m=treeView->model();
      const int modeCount=m->rowCount(QModelIndex());
      QString currentHlMode;
      KTextEditor::View *view=m_mainWindow->activeView();
      KTextEditor::HighlightInterface *fi=qobject_cast<KTextEditor::HighlightInterface*>(view->document());      
      if (fi)
      {
        currentHlMode=fi->highlightingModeAt(view->cursorPosition());
        //edit-selection is not the best choice, but the current version of the icon, an arrow is a good symbol for the default action now
        m_addSnippetToPopup->addAction(KIcon("edit-select"),i18n(ON_THE_GO_TEMPLATESTR,currentHlMode));
      } else kDebug()<<"document does not implement the highlight interface";
      KTextEditor::CodesnippetsCore::SnippetRepositoryModel *repo=m_plugin->repositoryData();
      for (int i=0;i<modeCount;i++) {
        QModelIndex mergedRepoIndex=m->index(i,0,QModelIndex());
        QString title=m->data(mergedRepoIndex,Qt::DisplayRole).toString();       
        QMenu  *menu=m_addSnippetToPopup->addMenu(title);
        //kDebug()<<"currentHlMode:"<<currentHlMode<<" title:"<<title;
        QStringList files=m->data(mergedRepoIndex, KTextEditor::CodesnippetsCore::SnippetSelectorModel::MergedFilesRole).toStringList();
        //kDebug()<<files;
        foreach (const QString& filename, files) {          
          //kDebug()<<"looking up filename: "<<filename;
          QModelIndex repoFileIdx=repo->indexForFile(filename);
          //kDebug()<<repoFileIdx;
          if (repoFileIdx.isValid())
          {
            menu->addAction(repo->data(repoFileIdx,KTextEditor::CodesnippetsCore::SnippetRepositoryModel::NameRole).toString());
          } else kDebug()<<"Filename not found in repository";
        }
      }
  }


}

// kate: space-indent on; indent-width 2; replace-tabs on;
