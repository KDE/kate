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

#include "jowennsnippets.h"
#include "jowennsnippets.moc"
#include "repository.h"
#include "selector.h"
#include <kate/documentmanager.h>
#include <kate/application.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/codecompletioninterface.h>

#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kpluginfactory.h>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kmessagebox.h>

K_PLUGIN_FACTORY(JoWennKateSnippetsFactory, registerPlugin<JoWenn::KateSnippetsPlugin>();)
K_EXPORT_PLUGIN(JoWennKateSnippetsFactory(KAboutData("katesnippets_tng","katesnippets_tng",ki18n("JOWENN's Snippets"), "0.1", ki18n("JOWENN's Snippets"), KAboutData::License_LGPL)) )

namespace JoWenn {
  
//BEGIN: PLUGIN  
  
  KateSnippetsPlugin::KateSnippetsPlugin( QObject* parent, const QList<QVariant>& ):
      Kate::Plugin ( (Kate::Application*)parent )
  {
    m_repositoryData=new KateSnippetRepositoryModel(this);
    connect(m_repositoryData,SIGNAL(typeChanged(const QString&)),this,SLOT(slotTypeChanged(const QString&)));
    
    Kate::DocumentManager* documentManager=application()->documentManager();
    const QList<KTextEditor::Document*>& documents=documentManager->documents ();
    foreach(KTextEditor::Document* document,documents) {
      addDocument(document);
    }
    connect(documentManager,SIGNAL(documentCreated (KTextEditor::Document *)),this,SLOT(addDocument(KTextEditor::Document*)));
    connect(documentManager,SIGNAL(documentWillBeDeleted (KTextEditor::Document *)),this,SLOT(removeDocument(KTextEditor::Document*)));
  }


  KateSnippetsPlugin::~KateSnippetsPlugin()
  {
    m_document_model_hash.clear();
    m_mode_model_hash.clear();
  }


  void KateSnippetsPlugin::addDocument(KTextEditor::Document* document)
  {
    QSharedPointer<KTextEditor::CodeCompletionModel2> completionModel;
    QHash<QString,QWeakPointer<KTextEditor::CodeCompletionModel2> >::iterator it=m_mode_model_hash.find(document->mode());
    if (it!=m_mode_model_hash.end()) {
      completionModel=it.value();
    }
    if (completionModel.isNull()) {
      completionModel=QSharedPointer<KTextEditor::CodeCompletionModel2>(m_repositoryData->completionModel(document->mode()));
      m_mode_model_hash.insert(document->mode(),completionModel);
    }
    m_document_model_hash.insert(document,QSharedPointer<KTextEditor::CodeCompletionModel2>(completionModel));
    const QList<KTextEditor::View*>& views=document->views();
    foreach (KTextEditor::View *view,views) {
      addView(document,view);
    }
    connect(document,SIGNAL(modeChanged (KTextEditor::Document *)),this,SLOT(updateDocument(KTextEditor::Document*)));      
    connect(document,SIGNAL(viewCreated (KTextEditor::Document *, KTextEditor::View *)),this,SLOT(addView(KTextEditor::Document*,KTextEditor::View*)));      
  }

  void KateSnippetsPlugin::removeDocument(KTextEditor::Document* document)
  {
    QSharedPointer<KTextEditor::CodeCompletionModel2>model=m_document_model_hash.take(document);
    const QList<KTextEditor::View*>& views=document->views();
    foreach (KTextEditor::View *view,views) {
      KTextEditor::CodeCompletionInterface *iface =
        qobject_cast<KTextEditor::CodeCompletionInterface*>( view );
      if (iface) {
        iface->unregisterCompletionModel(model.data());
      }
    }
  }

  JoWenn::KateSnippetCompletionModel* KateSnippetsPlugin::modelForDocument(KTextEditor::Document *document)
  {
    QSharedPointer<KTextEditor::CodeCompletionModel2>model=m_document_model_hash[document];
    return (KateSnippetCompletionModel*)model.data();
  }

  void KateSnippetsPlugin::addView(KTextEditor::Document* document,KTextEditor::View* view)
  {
    QSharedPointer<KTextEditor::CodeCompletionModel2>model=m_document_model_hash[document];
    KTextEditor::CodeCompletionInterface *iface =
      qobject_cast<KTextEditor::CodeCompletionInterface*>( view );
    if (iface) {
      iface->unregisterCompletionModel(model.data());
      iface->registerCompletionModel(model.data());
    }
  }

  void KateSnippetsPlugin::updateDocument(KTextEditor::Document* document)
  {
    QSharedPointer<KTextEditor::CodeCompletionModel2>model_d=m_document_model_hash[document];
    QSharedPointer<KTextEditor::CodeCompletionModel2>model_t=m_mode_model_hash[document->mode()];
    if (model_t==model_d) return;
    removeDocument(document);
    addDocument(document);
  }

  void KateSnippetsPlugin::slotTypeChanged(const QString& fileType)
  {
    QList<KTextEditor::Document*> refreshList;
    if (fileType=="*") {
      refreshList=m_document_model_hash.keys();
    } else {
      QSharedPointer<KTextEditor::CodeCompletionModel2>model_t=m_mode_model_hash[fileType];
      QHash<KTextEditor::Document*,QSharedPointer<KTextEditor::CodeCompletionModel2> >::const_iterator it;
      for(it=m_document_model_hash.constBegin();it!=m_document_model_hash.constEnd();++it) {
        if (it.value()==model_t) {
          refreshList<<it.key();
        }
      }
    }
    foreach(KTextEditor::Document* doc,refreshList) {
      removeDocument(doc);
    }
    foreach(KTextEditor::Document* doc,refreshList) {
      addDocument(doc);
    }
  }

  Kate::PluginView *KateSnippetsPlugin::createView (Kate::MainWindow *mainWindow)
  {
    KateSnippetsPluginView *view = new KateSnippetsPluginView (mainWindow,this);
    mViews.append( view );
    return view;
  }

  Kate::PluginConfigPage *KateSnippetsPlugin::configPage (uint number, QWidget *parent, const char *name)
  {
    Q_UNUSED(name)
    if (number != 0)
      return 0;
    return new KateSnippetsConfigPage(parent, this);
  }

  QString KateSnippetsPlugin::configPageName (uint number) const
  {
    if (number != 0) return QString();
    return i18n("JOWENN's Kate Snippets");
  }

  QString KateSnippetsPlugin::configPageFullName (uint number) const
  {
    if (number != 0) return QString();
    return i18n("JOWENN's Kate Snippets Settings");
  }

  KIcon KateSnippetsPlugin::configPageIcon (uint number) const
  {
    if (number != 0) return KIcon();
    return KIcon(/*Icon name here*/);
  }


  void KateSnippetsPlugin::readSessionConfig (KConfigBase* config, const QString& groupPrefix)
  {
    repositoryData()->readSessionConfig(config,groupPrefix);
    slotTypeChanged("*");       
  }
  
  void KateSnippetsPlugin::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
  {
    repositoryData()->writeSessionConfig(config,groupPrefix);
  }

  KateSnippetRepositoryModel * KateSnippetsPlugin::repositoryData() {
    return m_repositoryData;
  }

//END: PLUGIN

//BEGIN: VIEW

  KateSnippetsPluginView::KateSnippetsPluginView (Kate::MainWindow *mainWindow, JoWenn::KateSnippetsPlugin *plugin)
      : Kate::PluginView (mainWindow)
  {
    QWidget *toolview = mainWindow->createToolView ("jowenn_kate_plugin_snippets_tng", Kate::MainWindow::Left, SmallIcon("text-field"), i18n("JOWENN's Kate Snippets"));
    m_snippetSelector = new KateSnippetSelector(mainWindow, plugin, toolview);
  }

  KateSnippetsPluginView::~KateSnippetsPluginView ()
  {
     delete m_snippetSelector->parentWidget();
  }

//END: VIEW

//BEGIN: CONFIG PAGE

  KateSnippetsConfigPage::KateSnippetsConfigPage( QWidget* parent, KateSnippetsPlugin *plugin )
    : Kate::PluginConfigPage( parent )
    , m_plugin( plugin )
  {
    setupUi(this);
    KateSnippetRepositoryItemDelegate *delegate=new KateSnippetRepositoryItemDelegate(lstSnippetFiles,this);
    lstSnippetFiles->setItemDelegate(delegate);
    
    lstSnippetFiles->setModel(plugin->repositoryData());
    connect(btnNew,SIGNAL(clicked()),plugin->repositoryData(),SLOT(newEntry()));
    connect(btnCopy,SIGNAL(clicked()),this,SLOT(slotCopy()));
  }

  void KateSnippetsConfigPage::apply()
  {
    KConfigGroup config(KGlobal::config(), "JoWennSnippets");
    //config.writeEntry("AutoSyncronize", cbAutoSyncronize->isChecked());
    config.sync();    
  }

  void KateSnippetsConfigPage::reset()
  {
    KConfigGroup config(KGlobal::config(), "JoWennSnippets");
    //cbAutoSyncronize->setChecked(config.readEntry("AutoSyncronize", false));
  }

  void KateSnippetsConfigPage::slotCopy()
  {
    KUrl url(urlSource->url());
    if (!url.isValid()) return;
    m_plugin->repositoryData()->copyToRepository(url);
  }
  

//END: CONFIG PAGE

}
// kate: space-indent on; indent-width 2; replace-tabs on;

