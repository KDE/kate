/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes

#include "katefiletreeplugin.h"
#include "katefiletreeplugin.moc"
#include "katefiletree.h"
#include "katefiletreemodel.h"
#include "katefiletreeproxymodel.h"
#include "katefiletreeconfigpage.h"

#include <kate/application.h>
#include <kate/mainwindow.h>
#include <kate/documentmanager.h>
#include <ktexteditor/view.h>

#include <kaboutdata.h>
#include <kpluginfactory.h>

#include <KAction>
#include <KActionCollection>

#include <KConfigGroup>

#include "katefiletreedebug.h"

//END Includes

K_PLUGIN_FACTORY(KateFileTreeFactory, registerPlugin<KateFileTreePlugin>();)
K_EXPORT_PLUGIN(KateFileTreeFactory(KAboutData("katefiletreeplugin","katefiletreeplugin",ki18n("Document Tree"), "0.1", ki18n("Show open documents in a tree"), KAboutData::License_LGPL_V2)) )

//BEGIN KateFileTreePlugin
KateFileTreePlugin::KateFileTreePlugin(QObject* parent, const QList<QVariant>&)
  : Kate::Plugin ((Kate::Application*)parent)
  , m_view(0)
{

}

Kate::PluginView *KateFileTreePlugin::createView (Kate::MainWindow *mainWindow)
{
  m_view = new KateFileTreePluginView (mainWindow);
  return m_view;
}

uint KateFileTreePlugin::configPages() const
{
  return 1;
}


QString KateFileTreePlugin::configPageName (uint number) const
{
  if(number != 0)
    return QString();
  
  return QString("Tree View");
}

QString KateFileTreePlugin::configPageFullName (uint number) const
{
  if(number != 0)
    return QString();
    
  return QString("Configure Tree View");
}

KIcon KateFileTreePlugin::configPageIcon (uint number) const
{
  if(number != 0)
    return KIcon();
    
  return KIcon("view-list-tree");
}

Kate::PluginConfigPage *KateFileTreePlugin::configPage (uint number, QWidget *parent, const char *name)
{
  Q_UNUSED(name);
  if(number != 0)
    return 0;

  KateFileTreeConfigPage *page = new KateFileTreeConfigPage(parent, m_view);
  return page;
}

//END KateFileTreePlugin




//BEGIN KateFileTreePluginView
KateFileTreePluginView::KateFileTreePluginView (Kate::MainWindow *mainWindow)
: Kate::PluginView (mainWindow), KXMLGUIClient()
{
  // init console
  QWidget *toolview = mainWindow->createToolView ("kate_private_plugin_katefiletreeplugin", Kate::MainWindow::Left, SmallIcon("document-open"), i18n("Document Tree"));
  m_fileTree = new KateFileTree(toolview);
  m_fileTree->setSortingEnabled(true);
  
  connect(m_fileTree, SIGNAL(activateDocument(KTextEditor::Document*)),
          this, SLOT(activateDocument(KTextEditor::Document*)));

  connect(m_fileTree, SIGNAL(viewModeChanged(bool)), this, SLOT(viewModeChanged(bool)));
  connect(m_fileTree, SIGNAL(sortRoleChanged(int)), this, SLOT(sortRoleChanged(int)));
  
  m_documentModel = new KateFileTreeModel(this);
  m_proxyModel = new KateFileTreeProxyModel(this);
  m_proxyModel->setSourceModel(m_documentModel);
  m_proxyModel->setDynamicSortFilter(true);
  
  Kate::DocumentManager *dm = Kate::application()->documentManager();
  
  connect(dm, SIGNAL(documentCreated(KTextEditor::Document *)),
          m_documentModel, SLOT(documentOpened(KTextEditor::Document *)));
  connect(dm, SIGNAL(documentWillBeDeleted(KTextEditor::Document *)),
          m_documentModel, SLOT(documentClosed(KTextEditor::Document *)));

  connect(dm, SIGNAL(documentCreated(KTextEditor::Document *)),
          this, SLOT(documentOpened(KTextEditor::Document *)));
  connect(dm, SIGNAL(documentWillBeDeleted(KTextEditor::Document *)),
          this, SLOT(documentClosed(KTextEditor::Document *)));
          
  m_fileTree->setModel(m_proxyModel);
  
  m_fileTree->setDragEnabled(false);
  m_fileTree->setDragDropMode(QAbstractItemView::InternalMove);
  m_fileTree->setDropIndicatorShown(false);

  m_fileTree->setSelectionMode(QAbstractItemView::SingleSelection);

  connect( m_fileTree->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), m_fileTree, SLOT(slotCurrentChanged(const QModelIndex&, const QModelIndex&)));
  
  connect(mainWindow, SIGNAL(viewChanged()), this, SLOT(viewChanged()));

  setComponentData( KComponentData("kate") );
  setXMLFile(QString::fromLatin1("plugins/filetree/ui.rc"));
  
  KAction *show_active = actionCollection()->addAction("filetree_show_active_document", mainWindow);
  show_active->setText(i18n("&Show Active"));
  show_active->setIcon(KIcon("folder-sync"));
  show_active->setShortcut( QKeySequence(Qt::ALT+Qt::Key_A), KAction::DefaultShortcut );
  connect( show_active, SIGNAL( triggered(bool) ), this, SLOT( showActiveDocument() ) );

  mainWindow->guiFactory()->addClient(this);

  m_proxyModel->setSortRole(Qt::DisplayRole);
  
  m_proxyModel->sort(0, Qt::AscendingOrder);
  m_proxyModel->invalidate();
}

KateFileTreePluginView::~KateFileTreePluginView ()
{
  // clean up tree and toolview
  delete m_fileTree->parentWidget();
  // and TreeModel
  delete m_documentModel;
}

KateFileTreeModel *KateFileTreePluginView::model()
{
  return m_documentModel;
}

KateFileTreeProxyModel *KateFileTreePluginView::proxy()
{
  return m_proxyModel;
}

void KateFileTreePluginView::documentOpened(KTextEditor::Document *doc)
{
  kDebug(debugArea()) << "open" << doc;
  connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document*)),
          m_documentModel, SLOT(documentEdited(KTextEditor::Document*)));
  
  m_proxyModel->invalidate();
}

void KateFileTreePluginView::documentClosed(KTextEditor::Document *doc)
{
  kDebug(debugArea()) << "close" << doc;
  m_proxyModel->invalidate();
}
    
void KateFileTreePluginView::viewChanged()
{
  kDebug(debugArea()) << "BEGIN!";
  
  KTextEditor::View *view = mainWindow()->activeView();
  if(!view)
    return;
  
  KTextEditor::Document *doc = view->document();
  QModelIndex index = m_proxyModel->docIndex(doc);
  kDebug(debugArea()) << "selected doc=" << doc << index;

  QString display = m_proxyModel->data(index, Qt::DisplayRole).toString();
  kDebug(debugArea()) << "display="<<display;

  // update the model on which doc is active
  m_documentModel->documentActivated(doc);
  
  m_fileTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
  
  m_fileTree->scrollTo(index);
  
  while(index != QModelIndex()) {
    m_fileTree->expand(index);
    index = index.parent();
  }

  kDebug(debugArea()) << "END!";
}

void KateFileTreePluginView::setListMode(bool listMode)
{
  kDebug(debugArea()) << "BEGIN";
  
  if(listMode) {
    kDebug(debugArea()) << "listMode";
    m_documentModel->setListMode(true);
    m_fileTree->setRootIsDecorated(false);
  }
  else {
    kDebug(debugArea()) << "treeMode";
    m_documentModel->setListMode(false);
    m_fileTree->setRootIsDecorated(true);
  }

  m_proxyModel->sort(0, Qt::AscendingOrder);
  m_proxyModel->invalidate();

  kDebug(debugArea()) << "END";
}

void KateFileTreePluginView::viewModeChanged(bool listMode)
{
  kDebug(debugArea()) << "BEGIN";
  setListMode(listMode);
  kDebug(debugArea()) << "END";
}

void KateFileTreePluginView::sortRoleChanged(int role)
{
  kDebug(debugArea()) << "BEGIN";
  m_proxyModel->setSortRole(role);
  m_proxyModel->invalidate();
  kDebug(debugArea()) << "END";
}

void KateFileTreePluginView::activateDocument(KTextEditor::Document *doc)
{
  mainWindow()->activateView(doc);
}

void KateFileTreePluginView::showActiveDocument()
{
  // hack?
  viewChanged();
  // FIXME: make the tool view show if it was hidden
}

void KateFileTreePluginView::readSessionConfig(KConfigBase* config, const QString& group)
{
  KConfigGroup g = config->group(group);
  bool listMode = g.readEntry("listMode", QVariant(false)).toBool();
  
  setListMode(listMode);

  int sortRole = g.readEntry("sortRole", int(Qt::DisplayRole));
  m_proxyModel->setSortRole(sortRole);
  
//  m_fileTree->readSessionConfig(config, group);
}

void KateFileTreePluginView::writeSessionConfig(KConfigBase* config, const QString& group)
{
  KConfigGroup g = config->group(group);

  g.writeEntry("listMode", QVariant(m_documentModel->listMode()));
  g.writeEntry("sortRole", int(m_proxyModel->sortRole()));

  g.sync();
//  m_fileTree->writeSessionConfig(config, group);
}
//ENDKateFileTreePluginView


// kate: space-indent on; indent-width 2; replace-tabs on;
