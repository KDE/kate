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
#include <ktexteditor/templateinterface2.h>
#include <ktexteditor/highlightinterface.h>
#include <kate/pluginconfigpageinterface.h>

#include <kdebug.h>
#include <qmenu.h>
#include <kmessagebox.h>

#define ON_THE_GO_TEMPLATESTR "%1 On-The-Go"

namespace JoWenn {
  
  KateSnippetSelector::KateSnippetSelector(Kate::MainWindow *mainWindow,JoWenn::KateSnippetsPlugin *plugin, QWidget *parent):QWidget(parent),m_plugin(plugin),m_mainWindow(mainWindow),m_mode("_____")
  {    
    setupUi(this);
    plainTextEdit->setReadOnly(true);
    addSnippetToButton->setIcon(KIcon("snippetadd"));
    addSnippetToButton->setToolTip(i18n("Add current text selection to a snippet file (click=add to on-the-go, hold=menu=more options)"));
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
    connect(addSnippetToButton,SIGNAL(clicked()),this,SLOT(addSnippetToClicked()));
    connect(m_addSnippetToPopup,SIGNAL(aboutToShow()),this,SLOT(addSnippetToPopupAboutToShow()));
    connect(newRepoButton,SIGNAL(clicked()),this,SLOT(newRepo()));
    viewChanged();
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
    if (m_associatedView.isNull())
      disconnect(m_associatedView.data(),SIGNAL(selectionChanged(KTextEditor::View *)),this,SLOT(selectionChanged(KTextEditor::View *)));
    if (view)      
    {
      m_associatedView=view;
      connect(view,SIGNAL(selectionChanged(KTextEditor::View *)),this,SLOT(selectionChanged(KTextEditor::View *)));
      selectionChanged(view);
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
    KTextEditor::TemplateInterface2 *ti2=qobject_cast<KTextEditor::TemplateInterface2*>(view);
    if (ti2) {
      
      ti2->insertTemplateText (view->cursorPosition(), treeView->model()->data(current,KTextEditor::CodesnippetsCore::SnippetSelectorModel::FillInRole).toString(),QMap<QString,QString>(),
                               treeView->model()->data(current,KTextEditor::CodesnippetsCore::SnippetSelectorModel::ScriptTokenRole).toString());
    } else {
      KTextEditor::TemplateInterface *ti=qobject_cast<KTextEditor::TemplateInterface*>(view);
      if (ti)
        ti->insertTemplateText (view->cursorPosition(), treeView->model()->data(current,KTextEditor::CodesnippetsCore::SnippetSelectorModel::FillInRole).toString(),QMap<QString,QString>());
    }
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
      //clear the menu and do some sanity checks
      m_addSnippetToPopup->clear();
      if (!treeView->model()) return;
      //get out model and request the highlighting interface
      QAbstractItemModel *m=treeView->model();
      const int modeCount=m->rowCount(QModelIndex());
      QString currentHlMode;
      KTextEditor::View *view=m_mainWindow->activeView();
      KTextEditor::HighlightInterface *fi=qobject_cast<KTextEditor::HighlightInterface*>(view->document());
      
      QAction *quickAction=0;
      QString quickActionTitle;
      if (fi)
      { // highlighting interface found, adding quick action
        currentHlMode=fi->highlightingModeAt(view->cursorPosition());
        //edit-selection is not the best choice, but the current version of the icon, an arrow is a good symbol for the default action now
        QString on_the_go_title=i18n(ON_THE_GO_TEMPLATESTR,currentHlMode);
        quickActionTitle=i18n(ON_THE_GO_TEMPLATESTR,currentHlMode);
        quickAction=m_addSnippetToPopup->addAction(KIcon("edit-select"),quickActionTitle);
        QVariant v;
        v.setValue(ActionData("",currentHlMode));
        quickAction->setData(v);
        connect(quickAction,SIGNAL(triggered(bool)),this,SLOT(addSnippetToTriggered()));
      } else kDebug()<<"document does not implement the highlight interface";
      
      //highlighting interface is here, add all embedded highlightings to the menu
      KTextEditor::CodesnippetsCore::SnippetRepositoryModel *repo=m_plugin->repositoryData();
      for (int i=0;i<modeCount;i++) {
        // create a new menu for the given mode
        QModelIndex mergedRepoIndex=m->index(i,0,QModelIndex());
        QString title=m->data(mergedRepoIndex,Qt::DisplayRole).toString();       
        QMenu  *menu=m_addSnippetToPopup->addMenu(title);
      

        QString on_the_go_title=i18n(ON_THE_GO_TEMPLATESTR,title);
        //get all files merged for the current highlighting mode
        QStringList files=m->data(mergedRepoIndex, KTextEditor::CodesnippetsCore::SnippetSelectorModel::MergedFilesRole).toStringList();

        //iterate over all files of the current mode and add them to the menu
        bool on_the_go_found=false;
        foreach (const QString& filename, files) {          
          //lookup the file in the 
          QModelIndex repoFileIdx=repo->indexForFile(filename);
          //kDebug()<<repoFileIdx;
     
          if (repoFileIdx.isValid())           
          {
            //if the file is still in the repository, add the action to the  menu
            QString n=repo->data(repoFileIdx,KTextEditor::CodesnippetsCore::SnippetRepositoryModel::NameRole).toString();
            //if the title of the current file equals the on-the-go title, we don't have to add an additional lateron
            if (n==on_the_go_title)
            {
              on_the_go_found=true;
              if (quickActionTitle==n) //if the current on-the-go action equals the "global" one set the filename for it
              {
                if (quickAction) {
                  QVariant v;
                  v.setValue(ActionData(filename,""));
                  quickAction->setData(v);
                }
              }
            }
            QAction *a=menu->addAction(n);
            QVariant v;
            v.setValue(ActionData(filename,""));
            a->setData(v);
            connect(a,SIGNAL(triggered(bool)),this,SLOT(addSnippetToTriggered()));
          } else kDebug()<<"Filename not found in repository";
        }
        //for the current hl mode there was no active on the fly file active, add a new one, this will be autocreated
        if (!on_the_go_found) {
          QAction *a=new QAction(on_the_go_title,menu);
          QVariant v;
          v.setValue(ActionData("",title));
          a->setData(v);
          menu->insertAction(0,a);
          connect(a,SIGNAL(triggered(bool)),this,SLOT(addSnippetToTriggered()));
        }
      }
  }

  void KateSnippetSelector::addSnippetToClicked() {
      KTextEditor::View *view=m_mainWindow->activeView();
      KTextEditor::HighlightInterface *fi=qobject_cast<KTextEditor::HighlightInterface*>(view->document());      
      if (!fi) {
          KMessageBox::error(this,i18n("Developer's fault! Your editor component doesn't support the retrieval of certain\n"
                                       "information, please press this button longer to open the menu for manual\n"
                                       "destination selection"));
          return;
      }
      QString hlmode=fi->highlightingModeAt(view->cursorPosition());
      //Fill the menu and extract the on-the-go action
      addSnippetToPopupAboutToShow();
      if (m_addSnippetToPopup->actions().count()==0)
      {
        KMessageBox::error(this,i18n("Should not happen, cannot add snippet to a repository"));
        return;
      }
      addSnippetToAction(m_addSnippetToPopup->actions()[0]);
  }
  
  void KateSnippetSelector::addSnippetToTriggered() {
    addSnippetToAction(dynamic_cast<QAction*>(sender()));
  }
  
  void KateSnippetSelector::addSnippetToAction(QAction *action) {
      //retrieve name and filepath      
      QString filePath;
      if (action->data().isValid())
      {
        filePath=action->data().value<JoWenn::KateSnippetSelector::ActionData>().filePath;
      }
      KTextEditor::CodesnippetsCore::SnippetRepositoryModel *repo=m_plugin->repositoryData();
      if (filePath.isEmpty())       
      {
        //Only on-the-go may have an empty path.
        //If the on-the-go menu has no file attached, look if there is one in the repository and activate it.
        //If there is no one in the repository list, create a new one        
        QModelIndex index=repo->findFirstByName(action->text());
        if (index.isValid())
        {
          repo->setData(index,QVariant(true),KTextEditor::CodesnippetsCore::SnippetRepositoryModel::EnabledRole);
          filePath=repo->data(index,KTextEditor::CodesnippetsCore::SnippetRepositoryModel::FilenameRole).toString();
        } //create a new on the go
      }
      if (filePath.isEmpty()) {
        QString hlMode=action->data().value<JoWenn::KateSnippetSelector::ActionData>().hlMode;
        m_plugin->repositoryData()->addSnippetToNewEntry(this,m_mainWindow->activeView()->selectionText(),
        i18n(ON_THE_GO_TEMPLATESTR,hlMode),hlMode,true);
        return;
      }
      //we found the file
      repo->addSnippetToFile(this,m_mainWindow->activeView()->selectionText(),filePath);
  }

  void KateSnippetSelector::newRepo() {
      KTextEditor::View *view=m_mainWindow->activeView();
      KTextEditor::HighlightInterface *fi=qobject_cast<KTextEditor::HighlightInterface*>(view->document());      
      if (!fi) {
        m_plugin->repositoryData()->newEntry(this);        
      } else {
        m_plugin->repositoryData()->newEntry(this,fi->highlightingModeAt(view->cursorPosition()),true);
      }
  }

  void KateSnippetSelector::selectionChanged(KTextEditor::View *view) {
    bool enabled=addSnippetToButton->isEnabled();
    if (enabled!=view->selection()) {
      addSnippetToButton->setEnabled(!enabled);
      emit enableAdd(!enabled);
    }
  }

}

// kate: space-indent on; indent-width 2; replace-tabs on;