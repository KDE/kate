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
#include "lib/completionmodel.h"

#include <kate/documentmanager.h>
#include <kate/application.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/highlightinterface.h>

#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kpluginfactory.h>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kmessagebox.h>

#include <kactioncollection.h>
#include <kaction.h>

K_PLUGIN_FACTORY(JoWennKateSnippetsFactory, registerPlugin<JoWenn::KateSnippetsPlugin>();)
K_EXPORT_PLUGIN(JoWennKateSnippetsFactory(KAboutData("katesnippets_tng","katesnippets_tng",ki18n("Kate Snippets"), "0.1", ki18n("Kate Snippets"), KAboutData::License_LGPL)) )

namespace JoWenn {
  
//BEGIN: PLUGIN  
  
  KateSnippetsPlugin::KateSnippetsPlugin( QObject* parent, const QList<QVariant>& ):
      Kate::Plugin ( (Kate::Application*)parent )
  {
    KGlobal::locale()->insertCatalog("ktexteditor_codesnippets_core");
    m_repositoryData=new KTextEditor::CodesnippetsCore::SnippetRepositoryModel(this);
    connect(m_repositoryData,SIGNAL(typeChanged(const QStringList&)),this,SLOT(slotTypeChanged(const QStringList&)));
    
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
    m_document_model_multihash.clear();
    m_mode_model_hash.clear();
  }


  void KateSnippetsPlugin::addDocument(KTextEditor::Document* document)
  {
    KTextEditor::HighlightInterface *hli=qobject_cast<KTextEditor::HighlightInterface*>(document);
    if (!hli) return;
    QStringList modes;
    modes<<document->mode();
    modes<<hli->embeddedHighlightingModes();    
    kDebug()<<modes;
    kDebug()<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    QList<KTextEditor::CodesnippetsCore::SnippetCompletionModel> models;
    foreach (const QString& mode, modes)
    {    
      QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel> completionModel;
      QHash<QString,QWeakPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel> >::iterator it=m_mode_model_hash.find(mode);
      if (it!=m_mode_model_hash.end()) {
        completionModel=it.value();
      }
      if (completionModel.isNull()) {
        completionModel=QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel>(m_repositoryData->completionModel(mode));
        m_mode_model_hash.insert(mode,completionModel);
      }
      m_document_model_multihash.insert(document,QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel>(completionModel));
    }
    
    
    QList <QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel> >models2=m_document_model_multihash.values(document);
    QList<KTextEditor::CodesnippetsCore::SnippetSelectorModel*> list;
    foreach (const QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel>& model, models2)
    {
      list.append(model->selectorModel());
    }
    m_document_categorized_hash.insert(document,new KTextEditor::CodesnippetsCore::CategorizedSnippetModel(list));
    
    
    //Q_ASSERT(modelForDocument(document));
    const QList<KTextEditor::View*>& views=document->views();
    foreach (KTextEditor::View *view,views) {
      addView(document,view);
    }
    connect(document,SIGNAL(modeChanged (KTextEditor::Document *)),this,SLOT(updateDocument(KTextEditor::Document*)));      
    connect(document,SIGNAL(viewCreated (KTextEditor::Document *, KTextEditor::View *)),this,SLOT(addView(KTextEditor::Document*,KTextEditor::View*)));      
  }

  void KateSnippetsPlugin::removeDocument(KTextEditor::Document* document)
  {
    //if (!m_document_model_hash.contains(document)) return;
    delete m_document_categorized_hash.take(document);
    QList<QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel> >models=m_document_model_multihash.values(document);
    const QList<KTextEditor::View*>& views=document->views();
    foreach (const QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel>& model,models)
    {
      foreach (KTextEditor::View *view,views) {
        KTextEditor::CodeCompletionInterface *iface =
          qobject_cast<KTextEditor::CodeCompletionInterface*>( view );
        if (iface) {
          iface->unregisterCompletionModel(model.data());
        }
      }     
    }
    m_document_model_multihash.remove(document);
    Q_ASSERT(m_document_model_multihash.find(document)==m_document_model_multihash.end());
  }

  KTextEditor::CodesnippetsCore::CategorizedSnippetModel* KateSnippetsPlugin::modelForDocument(KTextEditor::Document *document)
  {
    return m_document_categorized_hash[document];
  }

  void KateSnippetsPlugin::addView(KTextEditor::Document* document,KTextEditor::View* view)
  {
    QList<QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel> > models=m_document_model_multihash.values(document);
    foreach (const QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel>& model, models) {
      KTextEditor::CodeCompletionInterface *iface =
        qobject_cast<KTextEditor::CodeCompletionInterface*>( view );
      if (iface) {
        iface->unregisterCompletionModel(model.data());
        iface->registerCompletionModel(model.data());
      }
    }
  }

  void KateSnippetsPlugin::updateDocument(KTextEditor::Document* document)
  {
/*    QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel>model_d=m_document_model_multihash[document];
    QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel>model_t=m_mode_model_hash[document->mode()];
    if (model_t==model_d) return;*/
    removeDocument(document);
    addDocument(document);
    kDebug()<<"invoking typeHasChanged(doc)";
    emit typeHasChanged(document);

  }

  void KateSnippetsPlugin::slotTypeChanged(const QStringList& fileType)
  {
    QSet<KTextEditor::Document*> refreshList;
    if (fileType.contains("*")) {
      QList<KTextEditor::Document*> tmp_list(m_document_model_multihash.keys());
      foreach(KTextEditor::Document *doc,tmp_list) {
      refreshList.insert(doc);
      }
    } else {
      foreach(QString ft, fileType) {
        QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel>model_t=m_mode_model_hash[ft];
        QMultiHash<KTextEditor::Document*,QSharedPointer<KTextEditor::CodesnippetsCore::SnippetCompletionModel> >::const_iterator it;
        for(it=m_document_model_multihash.constBegin();it!=m_document_model_multihash.constEnd();++it) {
          if (it.value()==model_t) {
            refreshList<<it.key();
          }
        }
      }
    }
    foreach(KTextEditor::Document* doc,refreshList) {
      removeDocument(doc);
    }
    foreach(KTextEditor::Document* doc,refreshList) {
      addDocument(doc);
      kDebug()<<"invoking typeHasChanged(doc)";
      emit typeHasChanged(doc);
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
    return i18n("Kate Snippets");
  }

  QString KateSnippetsPlugin::configPageFullName (uint number) const
  {
    if (number != 0) return QString();
    return i18n("Kate Snippets Settings");
  }

  KIcon KateSnippetsPlugin::configPageIcon (uint number) const
  {
    if (number != 0) return KIcon();
    return KIcon("textfield");
  }


  void KateSnippetsPlugin::readSessionConfig (KConfigBase* config, const QString& groupPrefix)
  {
    repositoryData()->readSessionConfig(config,groupPrefix);
    slotTypeChanged(QStringList("*"));       
  }
  
  void KateSnippetsPlugin::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
  {
    repositoryData()->writeSessionConfig(config,groupPrefix);
  }

  KTextEditor::CodesnippetsCore::SnippetRepositoryModel * KateSnippetsPlugin::repositoryData() {
    return m_repositoryData;
  }

//END: PLUGIN

//BEGIN: VIEW

  KateSnippetsPluginView::KateSnippetsPluginView (Kate::MainWindow *mainWindow, JoWenn::KateSnippetsPlugin *plugin)
      : Kate::PluginView (mainWindow), Kate::XMLGUIClient(JoWennKateSnippetsFactory::componentData())
  {
    QWidget *toolview = mainWindow->createToolView ("kate_plugin_snippets_tng", Kate::MainWindow::Left, SmallIcon("text-field"), i18n("Kate Snippets"));
    m_snippetSelector = new KateSnippetSelector(mainWindow, plugin, toolview);
    
    KAction *a=actionCollection()->addAction("popup_katesnippets_addto");
    a->setMenu(m_snippetSelector->addSnippetToPopup());    
    a->setIcon(KIcon("snippetadd"));
    a->setText(i18n("Create snippet"));
    mainWindow->guiFactory()->addClient (this);
    connect(m_snippetSelector,SIGNAL(enableAdd(bool)),a,SLOT(setEnabled(bool)));
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
    QVBoxLayout *l=new QVBoxLayout(this);
    KTextEditor::CodesnippetsCore::SnippetRepositoryConfigWidget *w=new KTextEditor::CodesnippetsCore::SnippetRepositoryConfigWidget(this,plugin->repositoryData());
    l->addWidget(w);
  }    
  
    void KateSnippetsConfigPage::apply()
  {
    KConfigGroup config(KGlobal::config(), "Kate Snippets");
    //config.writeEntry("AutoSyncronize", cbAutoSyncronize->isChecked());
    config.sync();    
  }

  void KateSnippetsConfigPage::reset()
  {
    KConfigGroup config(KGlobal::config(), "Kate Snippets");
    //cbAutoSyncronize->setChecked(config.readEntry("AutoSyncronize", false));
  }
//END: CONFIG PAGE

}
// kate: space-indent on; indent-width 2; replace-tabs on;

