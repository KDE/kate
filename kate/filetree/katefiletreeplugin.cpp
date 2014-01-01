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
#include "katefiletree.h"
#include "katefiletreemodel.h"
#include "katefiletreeproxymodel.h"
#include "katefiletreeconfigpage.h"

#include <ktexteditor/view.h>
#include <ktexteditor/application.h>

#include <KAboutData>
#include <KPluginFactory>
#include <KActionCollection>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KIconLoader>
#include <KXMLGUIFactory>

#include <QAction>
#include <QApplication>

#include "katefiletreedebug.h"

//END Includes

K_PLUGIN_FACTORY_WITH_JSON (KateFileTreeFactory, "katefiletreeplugin.json", registerPlugin<KateFileTreePlugin>();)

Q_LOGGING_CATEGORY(FILETREE, "kate-filetree")

//K_EXPORT_PLUGIN(KateFileTreeFactory(KAboutData("filetree","katefiletreeplugin",ki18n("Document Tree"), "0.1", ki18n("Show open documents in a tree"), KAboutData::License_LGPL_V2)) )

//BEGIN KateFileTreePlugin
KateFileTreePlugin::KateFileTreePlugin(QObject* parent, const QList<QVariant>&)
  : KTextEditor::ApplicationPlugin (parent),
    m_fileCommand(0)
{
// TODO: Remove once Qt allows an external app to change a debug category (ETA Qt 5.3)
  QLoggingCategory::setFilterRules(QStringLiteral("kate-filetree.debug = true"));

  KTextEditor::CommandInterface* iface =
  qobject_cast<KTextEditor::CommandInterface*>(KTextEditor::Editor::instance());
  if (iface) {
    m_fileCommand = new KateFileTreeCommand(this);
    iface->registerCommand(m_fileCommand);
  }
}

KateFileTreePlugin::~KateFileTreePlugin()
{
  m_settings.save();
  KTextEditor::CommandInterface* iface =
  qobject_cast<KTextEditor::CommandInterface*>(KTextEditor::Editor::instance());
  if (iface && m_fileCommand) {
    iface->unregisterCommand(m_fileCommand);
  }
}

QObject *KateFileTreePlugin::createView (KTextEditor::MainWindow *mainWindow)
{
  KateFileTreePluginView* view = new KateFileTreePluginView (mainWindow, this);
  connect(view, SIGNAL(destroyed(QObject*)), this, SLOT(viewDestroyed(QObject*)));
  connect(m_fileCommand, SIGNAL(showToolView()), view, SLOT(showToolView()));
  connect(m_fileCommand, SIGNAL(slotDocumentPrev()), view->tree(), SLOT(slotDocumentPrev()));
  connect(m_fileCommand, SIGNAL(slotDocumentNext()), view->tree(), SLOT(slotDocumentNext()));
  connect(m_fileCommand, SIGNAL(slotDocumentFirst()), view->tree(), SLOT(slotDocumentFirst()));
  connect(m_fileCommand, SIGNAL(slotDocumentLast()), view->tree(), SLOT(slotDocumentLast()));
  connect(m_fileCommand, SIGNAL(switchDocument(QString)), view->tree(), SLOT(switchDocument(QString)));
  m_views.append(view);

  return view;
}

void KateFileTreePlugin::viewDestroyed(QObject* view)
{
  // do not access the view pointer, since it is partially destroyed already
  m_views.removeAll(static_cast<KateFileTreePluginView *>(view));
}

int KateFileTreePlugin::configPages() const
{
  return 1;
}


QString KateFileTreePlugin::configPageName (int number) const
{
  if(number != 0)
    return QString();

  return QString(i18n("Documents"));
}

QString KateFileTreePlugin::configPageFullName (int number) const
{
  if(number != 0)
    return QString();

  return QString(i18n("Configure Documents"));
}

QIcon KateFileTreePlugin::configPageIcon (int number) const
{
  if(number != 0)
    return QIcon();

  return QIcon::fromTheme("view-list-tree");
}

KTextEditor::ConfigPage *KateFileTreePlugin::configPage (int number, QWidget *parent)
{
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
  foreach(KateFileTreePluginView *view, m_views) {
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
KateFileTreePluginView::KateFileTreePluginView (KTextEditor::MainWindow *mainWindow, KateFileTreePlugin *plug)
  : QObject (mainWindow), m_plug(plug), m_mainWindow (mainWindow)
{
  // FIXME KF5
  KXMLGUIClient::setComponentName ("katefiletree", i18n ("Kate File Tree"));
  setXMLFile( "ui.rc" );
  
  m_toolView = mainWindow->createToolView (plug,"kate_private_plugin_katefiletreeplugin", KTextEditor::MainWindow::Left, SmallIcon("document-open"), i18n("Documents"));
  m_fileTree = new KateFileTree(m_toolView);
  m_fileTree->setSortingEnabled(true);

  connect(m_fileTree, SIGNAL(activateDocument(KTextEditor::Document*)),
          this, SLOT(activateDocument(KTextEditor::Document*)));

  connect(m_fileTree, SIGNAL(viewModeChanged(bool)), this, SLOT(viewModeChanged(bool)));
  connect(m_fileTree, SIGNAL(sortRoleChanged(int)), this, SLOT(sortRoleChanged(int)));

  m_documentModel = new KateFileTreeModel(this);
  m_proxyModel = new KateFileTreeProxyModel(this);
  m_proxyModel->setSourceModel(m_documentModel);
  m_proxyModel->setDynamicSortFilter(true);

  m_documentModel->setShowFullPathOnRoots(m_plug->settings().showFullPathOnRoots());
  m_documentModel->setShadingEnabled(m_plug->settings().shadingEnabled());
  m_documentModel->setViewShade(m_plug->settings().viewShade());
  m_documentModel->setEditShade(m_plug->settings().editShade());

  connect(KTextEditor::Editor::instance()->application(), SIGNAL(documentCreated(KTextEditor::Document*)),
          m_documentModel, SLOT(documentOpened(KTextEditor::Document*)));
  connect(KTextEditor::Editor::instance()->application(), SIGNAL(documentWillBeDeleted(KTextEditor::Document*)),
          m_documentModel, SLOT(documentClosed(KTextEditor::Document*)));

  connect(KTextEditor::Editor::instance()->application(), SIGNAL(documentCreated(KTextEditor::Document*)),
          this, SLOT(documentOpened(KTextEditor::Document*)));
  connect(KTextEditor::Editor::instance()->application(), SIGNAL(documentWillBeDeleted(KTextEditor::Document*)),
          this, SLOT(documentClosed(KTextEditor::Document*)));

  connect(m_documentModel,SIGNAL(triggerViewChangeAfterNameChange()),this,SLOT(viewChanged()));
  m_fileTree->setModel(m_proxyModel);

  m_fileTree->setDragEnabled(false);
  m_fileTree->setDragDropMode(QAbstractItemView::InternalMove);
  m_fileTree->setDropIndicatorShown(false);

  m_fileTree->setSelectionMode(QAbstractItemView::SingleSelection);

  connect( m_fileTree->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), m_fileTree, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)));

  connect(mainWindow, SIGNAL(viewChanged(KTextEditor::View*)), this, SLOT(viewChanged(KTextEditor::View*)));

  QAction *show_active = actionCollection()->addAction("filetree_show_active_document", mainWindow);
  show_active->setText(i18n("&Show Active"));
  show_active->setIcon(QIcon::fromTheme("folder-sync"));
  connect( show_active, SIGNAL(triggered(bool)), this, SLOT(showActiveDocument()) );

  /**
   * back + forward
   */
  actionCollection()->addAction( KStandardAction::Back, "filetree_prev_document", m_fileTree, SLOT(slotDocumentPrev()) )->setText(i18n("Previous Document"));
  actionCollection()->addAction( KStandardAction::Forward, "filetree_next_document", m_fileTree, SLOT(slotDocumentNext()) )->setText(i18n("Next Document"));

  mainWindow->guiFactory()->addClient(this);

  m_proxyModel->setSortRole(Qt::DisplayRole);

  m_proxyModel->sort(0, Qt::AscendingOrder);
  m_proxyModel->invalidate();
}

KateFileTreePluginView::~KateFileTreePluginView ()
{
  m_mainWindow->guiFactory()->removeClient(this);

  // clean up tree and toolview
  delete m_fileTree->parentWidget();
  // delete m_toolView;
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

KateFileTree *KateFileTreePluginView::tree()
{
  return m_fileTree;
}

void KateFileTreePluginView::documentOpened(KTextEditor::Document *doc)
{
  connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document*)),
          m_documentModel, SLOT(documentEdited(KTextEditor::Document*)));

  m_proxyModel->invalidate();
}

void KateFileTreePluginView::documentClosed(KTextEditor::Document *doc)
{
  Q_UNUSED(doc);
  m_proxyModel->invalidate();
}

void KateFileTreePluginView::viewChanged(KTextEditor::View *)
{
  KTextEditor::View *view = m_mainWindow->activeView();
  if(!view)
    return;

  KTextEditor::Document *doc = view->document();
  QModelIndex index = m_proxyModel->docIndex(doc);

  QString display = m_proxyModel->data(index, Qt::DisplayRole).toString();

  // update the model on which doc is active
  m_documentModel->documentActivated(doc);

  m_fileTree->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);

  m_fileTree->scrollTo(index);

  while(index != QModelIndex()) {
    m_fileTree->expand(index);
    index = index.parent();
  }
}

void KateFileTreePluginView::setListMode(bool listMode)
{
  if(listMode) {
    m_documentModel->setListMode(true);
    m_fileTree->setRootIsDecorated(false);
  }
  else {
    m_documentModel->setListMode(false);
    m_fileTree->setRootIsDecorated(true);
  }

  m_proxyModel->sort(0, Qt::AscendingOrder);
  m_proxyModel->invalidate();
}

void KateFileTreePluginView::viewModeChanged(bool listMode)
{
  setHasLocalPrefs(true);
  setListMode(listMode);
}

void KateFileTreePluginView::sortRoleChanged(int role)
{
  setHasLocalPrefs(true);
  m_proxyModel->setSortRole(role);
  m_proxyModel->invalidate();
}

void KateFileTreePluginView::activateDocument(KTextEditor::Document *doc)
{
  m_mainWindow->activateView(doc);
}

void KateFileTreePluginView::showToolView()
{
  m_mainWindow->showToolView(m_toolView);
  m_toolView->setFocus();
}

void KateFileTreePluginView::hideToolView()
{
  m_mainWindow->hideToolView(m_toolView);
}

void KateFileTreePluginView::switchDocument(const QString &doc)
{
  m_fileTree->switchDocument(doc);
}

void KateFileTreePluginView::showActiveDocument()
{
  // hack?
  viewChanged();
  // make the tool view show if it was hidden
  showToolView();
}

bool KateFileTreePluginView::hasLocalPrefs()
{
  return m_hasLocalPrefs;
}

void KateFileTreePluginView::setHasLocalPrefs(bool h)
{
  m_hasLocalPrefs = h;
}

void KateFileTreePluginView::readSessionConfig (const KConfigGroup& g)
{
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

void KateFileTreePluginView::writeSessionConfig (KConfigGroup& g)
{
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
//END KateFileTreePluginView

//BEGIN KateFileTreeCommand
KateFileTreeCommand::KateFileTreeCommand(QObject *parent)
  : QObject(parent), KTextEditor::Command()
{
}

const QStringList& KateFileTreeCommand::cmds()
{
  static QStringList sl;

  if (sl.empty()) {
   sl << "ls"
      << "b" << "buffer"
      << "bn" << "bnext" << "bp" << "bprevious"
      << "tabn" << "tabnext" << "tabp" << "tabprevious"
      << "bf" << "bfirst" << "bl" << "blast"
      << "tabf" << "tabfirst" << "tabl" << "tablast";
  }

  return sl;
}

bool KateFileTreeCommand::exec(KTextEditor::View * /*view*/, const QString &cmd, QString & /*msg*/)
{
  // create list of args
  QStringList args(cmd.split(' ', QString::KeepEmptyParts));
  QString command = args.takeFirst(); // same as cmd if split failed
  QString argument = args.join(QString(' '));

  if (command == "ls") {
    emit showToolView();
  } else if (command == "b" || command == "buffer") {
    emit switchDocument(argument);
  } else if (command == "bp" || command == "bprevious" ||
             command == "tabp" || command == "tabprevious") {
    emit slotDocumentPrev();
  } else if (command == "bn" || command == "bnext" ||
             command == "tabn" || command == "tabnext") {
    emit slotDocumentNext();
  } else if (command == "bf" || command == "bfirst" ||
             command == "tabf" || command == "tabfirst") {
    emit slotDocumentFirst();
  } else if (command == "bl" || command == "blast" ||
             command == "tabl" || command == "tablast") {
    emit slotDocumentLast();
  }

  return true;
}

bool KateFileTreeCommand::help(KTextEditor::View * /*view*/, const QString &cmd, QString &msg)
{
  if (cmd == "b" || cmd == "buffer") {
      msg = i18n("<p><b>b,buffer &mdash; Edit document N from the document list</b></p>"
                 "<p>Usage: <tt><b>b[uffer] [N]</b></tt></p>");
      return true;
  } else if (cmd == "bp" || cmd == "bprevious" ||
             cmd == "tabp" || cmd == "tabprevious") {
      msg = i18n("<p><b>bp,bprev &mdash; previous buffer</b></p>"
                 "<p>Usage: <tt><b>bp[revious] [N]</b></tt></p>"
                 "<p>Goes to <b>[N]</b>th previous document (\"<b>b</b>uffer\") in document list. </p>"
                 "<p> <b>[N]</b> defaults to one. </p>"
                 "<p>Wraps around the start of the document list.</p>");
      return true;
  } else if (cmd == "bn" || cmd == "bnext" ||
             cmd == "tabn" || cmd == "tabnext") {
      msg = i18n("<p><b>bn,bnext &mdash; switch to next document</b></p>"
                 "<p>Usage: <tt><b>bn[ext] [N]</b></tt></p>"
                 "<p>Goes to <b>[N]</b>th next document (\"<b>b</b>uffer\") in document list."
                 "<b>[N]</b> defaults to one. </p>"
                 "<p>Wraps around the end of the document list.</p>");
      return true;
  } else if (cmd == "bf" || cmd == "bfirst" ||
             cmd == "tabf" || cmd == "tabfirst") {
      msg = i18n("<p><b>bf,bfirst &mdash; first document</b></p>"
                 "<p>Usage: <tt><b>bf[irst]</b></tt></p>"
                 "<p>Goes to the <b>f</b>irst document (\"<b>b</b>uffer\") in document list.</p>");
      return true;
  } else if (cmd == "bl" || cmd == "blast" ||
             cmd == "tabl" || cmd == "tablast") {
      msg = i18n("<p><b>bl,blast &mdash; last document</b></p>"
                 "<p>Usage: <tt><b>bl[ast]</b></tt></p>"
                 "<p>Goes to the <b>l</b>ast document (\"<b>b</b>uffer\") in document list.</p>");
      return true;
  }

  return false;
}
//END KateFileTreeCommand

#include "katefiletreeplugin.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
