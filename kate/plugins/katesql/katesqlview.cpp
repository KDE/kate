/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

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

#include "katesqlview.h"
#include "sqlmanager.h"
#include "connectionmodel.h"
#include "textoutputwidget.h"
#include "dataoutputwidget.h"
#include "dataoutputmodel.h"
#include "dataoutputview.h"
#include "schemawidget.h"
#include "schemabrowserwidget.h"
#include "connectionwizard.h"

#include <kate/plugin.h>
#include <kate/mainwindow.h>
#include <kate/documentmanager.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kcombobox.h>
#include <kconfig.h>

#include <qstring.h>
#include <qsqlquery.h>
#include <QVBoxLayout>
#include <QApplication>

KateSQLView::KateSQLView(Kate::MainWindow *mw)
: Kate::PluginView (mw)
, KXMLGUIClient()
, m_manager (new SQLManager(this))
{
  m_textOutputToolView    = mw->createToolView("kate_private_plugin_katesql_textoutput",
                                               Kate::MainWindow::Bottom,
                                               SmallIcon("view-list-text"),
                                               i18n("SQL Text Output")
                                               );

  m_dataOutputToolView    = mw->createToolView("kate_private_plugin_katesql_dataoutput",
                                              Kate::MainWindow::Bottom,
                                              SmallIcon("view-form-table"),
                                              i18n("SQL Data Output")
                                              );

  m_schemaBrowserToolView = mw->createToolView("kate_private_plugin_katesql_schemabrowser",
                                              Kate::MainWindow::Left,
                                              SmallIcon("view-list-tree"),
                                              i18n("SQL Schema Browser")
                                              );

  m_textOutputWidget = new TextOutputWidget(m_textOutputToolView);
  m_dataOutputWidget = new DataOutputWidget(m_dataOutputToolView);
  m_schemaBrowserWidget = new SchemaBrowserWidget(m_schemaBrowserToolView);

  m_connectionsComboBox = new KComboBox(this);
  m_connectionsComboBox->setEditable(false);
  m_connectionsComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
  m_connectionsComboBox->setModel(m_manager->connectionModel());

//   setComponentData( KateSQLFactory::componentData() );
  setComponentData( KComponentData("kate") );

  setupActions();

  setXMLFile(QString::fromLatin1("plugins/katesql/ui.rc"));
  mainWindow()->guiFactory()->addClient(this);

  connect(m_manager, SIGNAL(error(const QString&)), this, SLOT(slotError(const QString&)));
  connect(m_manager, SIGNAL(success(const QString&)), this, SLOT(slotSuccess(const QString&)));
  connect(m_manager, SIGNAL(queryActivated(QSqlQuery&)), this, SLOT(slotQueryActivated(QSqlQuery&)));
  connect(m_manager, SIGNAL(connectionCreated(const QString&)), this, SLOT(slotConnectionCreated(const QString&)));
  connect(m_connectionsComboBox, SIGNAL(currentIndexChanged(const QString&)), m_schemaBrowserWidget->schemaWidget(), SLOT(buildTree(const QString&)));
}


KateSQLView::~KateSQLView()
{
  mainWindow()->guiFactory()->removeClient( this );

  delete m_textOutputToolView;
  delete m_dataOutputToolView;
  delete m_schemaBrowserToolView;

  delete m_manager;
}


void KateSQLView::setupActions()
{
  KAction* action;
  KActionCollection* collection = actionCollection();

  action = collection->addAction("connection_create");
  action->setText( i18n("Add connection...") );
  action->setIcon( KIcon("list-add") );
  connect( action , SIGNAL(triggered()) , this , SLOT(slotConnectionCreate()) );

  action = collection->addAction("connection_remove");
  action->setText( i18n("Remove connection") );
  action->setIcon( KIcon("list-remove") );
  connect( action , SIGNAL(triggered()) , this , SLOT(slotConnectionRemove()) );

  action = collection->addAction("connection_chooser");
  action->setText( i18n("Connection") );
  action->setDefaultWidget(m_connectionsComboBox);

  action = collection->addAction("query_run");
  action->setText( i18n("Run query") );
  action->setIcon( KIcon("quickopen") );
  action->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_E) );
  connect( action , SIGNAL(triggered()) , this , SLOT(slotRunQuery()));

  /// TODO: stop sql query
  //   action = collection->addAction("sql_stop");
  //   action->setText( i18n("Stop query") );
  //   action->setIcon( KIcon("process-stop") );
  //   action->setShortcut( QKeySequence(Qt::ALT + Qt::Key_F5) );
  //   connect( action , SIGNAL(triggered()) , this , SLOT(stopQuery()));
}


void KateSQLView::slotGlobalSettingsChanged()
{
  m_dataOutputWidget->model()->readConfig();
}


void KateSQLView::readSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
  KConfigGroup globalConfig(KGlobal::config(), "KateSQLPlugin");

  bool saveConnections = globalConfig.readEntry("SaveConnections", true);

  if (!saveConnections)
    return;

  KConfigGroup group(config, groupPrefix + ":connections");

  m_manager->loadConnections(&group);

  QString lastConnection = group.readEntry("LastUsed");

  if (m_connectionsComboBox->contains(lastConnection))
    m_connectionsComboBox->setCurrentItem(lastConnection);
}


void KateSQLView::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
  KConfigGroup group(config, groupPrefix + ":connections");

  group.deleteGroup();

  KConfigGroup globalConfig(KGlobal::config(), "KateSQLPlugin");
  bool saveConnections = globalConfig.readEntry("SaveConnections", true);

  if (saveConnections)
  {
    m_manager->saveConnections(&group);

    group.writeEntry("LastUsed", m_connectionsComboBox->currentText());
  }

  config->sync();
}


void KateSQLView::slotConnectionCreate()
{
  ConnectionWizard wizard(m_manager);

  if (wizard.exec() != QDialog::Accepted)
    return;
}


void KateSQLView::slotConnectionRemove()
{
  QString connection = m_connectionsComboBox->currentText();

  if (!connection.isEmpty())
  {
    /// FIXME: clear results only if the associated query was generated by this connection
    m_dataOutputWidget->clearResults();
    m_manager->removeConnection(connection);
  }
}


void KateSQLView::slotRunQuery()
{
  /// TODO:
  /// bind parameters dialog?

  QString connection = m_connectionsComboBox->currentText();

  if (connection.isEmpty())
  {
    slotConnectionCreate();
    return;
  }

  if (!mainWindow())
    return;

  KTextEditor::View *view = mainWindow()->activeView();

  if (!view)
    return;

  QString text = (view->selection()) ? view->selectionText() : view->document()->text();
  text = text.trimmed();

  if (text.isEmpty())
    return;

  m_manager->runQuery(text, connection);
}


void KateSQLView::slotError(const QString &message)
{
  mainWindow()->showToolView(m_textOutputToolView);

  m_textOutputWidget->showErrorMessage(message);
}


void KateSQLView::slotSuccess(const QString &message)
{
  mainWindow()->showToolView(m_textOutputToolView);

  m_textOutputWidget->showSuccessMessage(message);
}


void KateSQLView::slotQueryActivated(QSqlQuery &query)
{
  if (query.isSelect())
    mainWindow()->showToolView(m_dataOutputToolView);

  m_dataOutputWidget->showQueryResultSets(query);
}


void KateSQLView::slotConnectionCreated(const QString &name)
{
  m_connectionsComboBox->setCurrentItem(name);

  m_schemaBrowserWidget->schemaWidget()->buildTree(name);
}

//END KateSQLView

