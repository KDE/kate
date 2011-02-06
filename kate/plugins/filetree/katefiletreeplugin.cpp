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

#include <QApplication>

#include "katefiletreedebug.h"

//END Includes

K_PLUGIN_FACTORY(KateFileTreeFactory, registerPlugin<KateFileTreePlugin>();)
K_EXPORT_PLUGIN(KateFileTreeFactory(KAboutData("filetree","katefiletreeplugin",ki18n("Document Tree"), "0.1", ki18n("Show open documents in a tree"), KAboutData::License_LGPL_V2)) )

//BEGIN KateFileTreePlugin
KateFileTreePlugin::KateFileTreePlugin(QObject* parent, const QList<QVariant>&)
  : Kate::Plugin ((Kate::Application*)parent)
{

}

KateFileTreePlugin::~KateFileTreePlugin()
{
  m_settings.save();
}

Kate::PluginView *KateFileTreePlugin::createView (Kate::MainWindow *mainWindow)
{
  if(m_view.contains(mainWindow)) {
    kDebug(debugArea()) << "ERROR: view hash already contains this mainWindow";
    Q_ASSERT(m_view.contains(mainWindow) == true);
  }

  m_view[mainWindow] = new KateFileTreePluginView (mainWindow, this);
  return m_view[mainWindow];
}

uint KateFileTreePlugin::configPages() const
{
  return 1;
}


QString KateFileTreePlugin::configPageName (uint number) const
{
  if(number != 0)
    return QString();

  return QString(i18n("Tree View"));
}

QString KateFileTreePlugin::configPageFullName (uint number) const
{
  if(number != 0)
    return QString();

  return QString(i18n("Configure Tree View"));
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

  KateFileTreeConfigPage *page = new KateFileTreeConfigPage(parent, this);
  return page;
}

const KateFileTreePluginSettings &KateFileTreePlugin::settings()
{
  return m_settings;
}

void KateFileTreePlugin::applyConfig(bool shadingEnabled, QColor viewShade, QColor editShade, bool listMode, int sortRole, bool showFullPath)
{
  // save to settings
  m_settings.setShadingEnabled(shadingEnabled);
  m_settings.setViewShade(viewShade);
  m_settings.setEditShade(editShade);

  m_settings.setListMode(listMode);
  m_settings.setSortRole(sortRole);
  m_settings.setShowFullPathOnRoots(showFullPath);
  m_settings.save();

  // update views
  foreach(KateFileTreePluginView *view, m_view.values()) {
    view->setHasLocalPrefs(false);
    view->model()->setShadingEnabled( shadingEnabled );
    view->model()->setViewShade( viewShade );
    view->model()->setEditShade( editShade );
    view->setListMode( listMode );
    view->proxy()->setSortRole( sortRole );
    view->model()->setShowFullPathOnRoots( showFullPath );
  }
}


//END KateFileTreePlugin




//BEGIN KateFileTreePluginView
KateFileTreePluginView::KateFileTreePluginView (Kate::MainWindow *mainWindow, KateFileTreePlugin *plug)
: Kate::PluginView (mainWindow), Kate::XMLGUIClient(KateFileTreeFactory::componentData()), m_plug(plug)
{
  // init console
  kDebug(debugArea()) << "BEGIN: mw:" << mainWindow;

  QWidget *toolview = mainWindow->createToolView ("kate_private_plugin_katefiletreeplugin", Kate::MainWindow::Left, SmallIcon("document-open"), i18n("Documents"));
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

  KAction *show_active = actionCollection()->addAction("filetree_show_active_document", mainWindow);
  show_active->setText(i18n("&Show Active"));
  show_active->setIcon(KIcon("folder-sync"));
  show_active->setShortcut( QKeySequence(Qt::ALT+Qt::Key_A), KAction::DefaultShortcut );
  connect( show_active, SIGNAL( triggered(bool) ), this, SLOT( showActiveDocument() ) );

  KAction *prev_document = actionCollection()->addAction("filetree_prev_document", mainWindow);
  prev_document->setText(i18n("&Prev Document"));
  prev_document->setIcon(KIcon("go-previous"));
  prev_document->setShortcut( QKeySequence(Qt::ALT+Qt::Key_Left), KAction::DefaultShortcut );
  connect( prev_document, SIGNAL( triggered(bool) ), m_fileTree, SLOT( slotDocumentPrev() ) );

  KAction *next_document = actionCollection()->addAction("filetree_next_document", mainWindow);
  next_document->setText(i18n("&Next Document"));
  next_document->setIcon(KIcon("go-next"));
  next_document->setShortcut( QKeySequence(Qt::ALT+Qt::Key_Right), KAction::DefaultShortcut );
  connect( next_document, SIGNAL( triggered(bool) ), m_fileTree, SLOT( slotDocumentNext() ) );

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
  setHasLocalPrefs(true);
  setListMode(listMode);
  kDebug(debugArea()) << "END";
}

void KateFileTreePluginView::sortRoleChanged(int role)
{
  kDebug(debugArea()) << "BEGIN";
  setHasLocalPrefs(true);
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

bool KateFileTreePluginView::hasLocalPrefs()
{
  return m_hasLocalPrefs;
}

void KateFileTreePluginView::setHasLocalPrefs(bool h)
{
  m_hasLocalPrefs = h;
}

void KateFileTreePluginView::readSessionConfig(KConfigBase* config, const QString& group)
{
  KConfigGroup g = config->group(group);

  if(g.exists())
    m_hasLocalPrefs = true;
  else
    m_hasLocalPrefs = false;

  // we chain to the global settings by using them as the defaults
  //  here in the session view config loading.
  const KateFileTreePluginSettings &defaults = m_plug->settings();

  bool listMode = g.readEntry("listMode", defaults.listMode());

  setListMode(listMode);

  int sortRole = g.readEntry("sortRole", defaults.sortRole());
  m_proxyModel->setSortRole(sortRole);

}

void KateFileTreePluginView::writeSessionConfig(KConfigBase* config, const QString& group)
{
  KConfigGroup g = config->group(group);

  if(m_hasLocalPrefs) {
    g.writeEntry("listMode", QVariant(m_documentModel->listMode()));
    g.writeEntry("sortRole", int(m_proxyModel->sortRole()));
  }
  else {
    g.deleteEntry("listMode");
    g.deleteEntry("sortRole");
  }

  g.sync();
}
//ENDKateFileTreePluginView

// kate: space-indent on; indent-width 2; replace-tabs on;
