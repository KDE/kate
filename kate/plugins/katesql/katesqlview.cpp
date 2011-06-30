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
#include "katesqlplugin.h"
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
#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kcombobox.h>
#include <kconfig.h>

#include <qmenu.h>
#include <qstring.h>
#include <qsqlquery.h>
#include <QVBoxLayout>
#include <QApplication>

KateSQLView::KateSQLView(Kate::MainWindow *mw)
: Kate::PluginView (mw)
, Kate::XMLGUIClient(KateSQLFactory::componentData())
, m_manager (new SQLManager(this))
{
  m_textOutputToolView    = mw->createToolView("kate_private_plugin_katesql_textoutput",
                                               Kate::MainWindow::Bottom,
                                               SmallIcon("view-list-text"),
                                               i18nc("@title:window", "SQL Text Output")
                                               );

  m_dataOutputToolView    = mw->createToolView("kate_private_plugin_katesql_dataoutput",
                                               Kate::MainWindow::Bottom,
                                               SmallIcon("view-form-table"),
                                               i18nc("@title:window", "SQL Data Output")
                                               );

  m_schemaBrowserToolView = mw->createToolView("kate_private_plugin_katesql_schemabrowser",
                                               Kate::MainWindow::Left,
                                               SmallIcon("view-list-tree"),
                                               i18nc("@title:window", "SQL Schema Browser")
                                               );

  m_textOutputWidget = new TextOutputWidget(m_textOutputToolView);
  m_dataOutputWidget = new DataOutputWidget(m_dataOutputToolView);
  m_schemaBrowserWidget = new SchemaBrowserWidget(m_schemaBrowserToolView, m_manager);

  m_connectionsComboBox = new KComboBox(this);
  m_connectionsComboBox->setEditable(false);
  m_connectionsComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
  m_connectionsComboBox->setModel(m_manager->connectionModel());
//   m_connectionsComboBox->setItemDelegate( new ConnectionDelegate(this) );

  setupActions();

  mainWindow()->guiFactory()->addClient(this);

  QMenu *sqlMenu = (QMenu*)factory()->container("SQL", this);

  m_connectionsGroup = new QActionGroup(sqlMenu);
  m_connectionsGroup->setExclusive(true);

  connect(sqlMenu, SIGNAL(aboutToShow()), this, SLOT(slotSQLMenuAboutToShow()));
  connect(m_connectionsGroup, SIGNAL(triggered(QAction *)), this, SLOT(slotConnectionSelectedFromMenu(QAction *)));

  connect(m_manager, SIGNAL(error(const QString&)), this, SLOT(slotError(const QString&)));
  connect(m_manager, SIGNAL(success(const QString&)), this, SLOT(slotSuccess(const QString&)));
  connect(m_manager, SIGNAL(queryActivated(QSqlQuery&, const QString&)), this, SLOT(slotQueryActivated(QSqlQuery&, const QString&)));
  connect(m_manager, SIGNAL(connectionCreated(const QString&)), this, SLOT(slotConnectionCreated(const QString&)));
  connect(m_connectionsComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slotConnectionChanged(const QString&)));

  stateChanged("has_connection_selected", KXMLGUIClient::StateReverse);
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
  action->setText( i18nc("@action:inmenu", "Add connection...") );
  action->setIcon( KIcon("list-add") );
  connect( action , SIGNAL(triggered()) , this , SLOT(slotConnectionCreate()) );

  action = collection->addAction("connection_remove");
  action->setText( i18nc("@action:inmenu", "Remove connection") );
  action->setIcon( KIcon("list-remove") );
  connect( action , SIGNAL(triggered()) , this , SLOT(slotConnectionRemove()) );

  action = collection->addAction("connection_edit");
  action->setText( i18nc("@action:inmenu", "Edit connection...") );
  action->setIcon( KIcon("configure") );
  connect( action , SIGNAL(triggered()) , this , SLOT(slotConnectionEdit()) );

  action = collection->addAction("connection_chooser");
  action->setText( i18nc("@action:intoolbar", "Connection") );
  action->setDefaultWidget(m_connectionsComboBox);

  action = collection->addAction("query_run");
  action->setText( i18nc("@action:inmenu", "Run query") );
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


void KateSQLView::slotSQLMenuAboutToShow()
{
  qDeleteAll( m_connectionsGroup->actions() );

  QMenu *sqlMenu = (QMenu*)factory()->container("SQL", this);
  QAction *before = action("query_run");
  QAbstractItemModel *model = m_manager->connectionModel();

  int rows = model->rowCount(QModelIndex());

  for (int row = 0; row < rows; row++)
  {
    QModelIndex index = model->index(row, 0, QModelIndex());

    Q_ASSERT(index.isValid());

    QString connectionName = index.data(Qt::DisplayRole).toString();

    QAction *act = new QAction(connectionName, m_connectionsGroup);
    act->setCheckable(true);

    if (m_connectionsComboBox->currentText() == connectionName)
      act->setChecked(true);

    sqlMenu->insertAction(before, act);
  }

  sqlMenu->insertSeparator(before);
}


void KateSQLView::slotConnectionSelectedFromMenu(QAction *action)
{
  m_connectionsComboBox->setCurrentItem(action->text());
}


void KateSQLView::slotConnectionChanged(const QString &connection)
{
  stateChanged("has_connection_selected", (connection.isEmpty()) ? KXMLGUIClient::StateReverse : KXMLGUIClient::StateNoReverse);

  m_schemaBrowserWidget->schemaWidget()->buildTree(connection);
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
  Connection c;

  ConnectionWizard wizard(m_manager, &c);

  if (wizard.exec() != QDialog::Accepted)
    return;

  for (int i = 1; QSqlDatabase::contains(c.name); i++)
    c.name = QString("%1 (%2)").arg(c.name).arg(i);

  m_manager->createConnection(c);

  if (m_manager->storeCredentials(c) != 0)
    kWarning() << "Connection credentials not saved";
}


void KateSQLView::slotConnectionEdit()
{
  int i = m_connectionsComboBox->currentIndex();

  if (i == -1)
    return;

  ConnectionModel *model = m_manager->connectionModel();
  Connection c = qVariantValue<Connection>(model->data(model->index(i), Qt::UserRole));

  QString previousName = c.name;

  ConnectionWizard wizard(m_manager, &c);

  if (wizard.exec() != QDialog::Accepted)
    return;

  /// i need to delete the QSqlQuery object inside the model before closing connection
  if (previousName == m_currentResultsetConnection)
    m_dataOutputWidget->clearResults();

  m_manager->removeConnection(previousName);
  m_manager->createConnection(c);

  if (m_manager->storeCredentials(c) != 0)
    kWarning() << "Connection credentials not saved";
}


void KateSQLView::slotConnectionRemove()
{
  QString connection = m_connectionsComboBox->currentText();

  if (!connection.isEmpty())
  {
    /// i need to delete the QSqlQuery object inside the model before closing connection
    if (connection == m_currentResultsetConnection)
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


void KateSQLView::slotQueryActivated(QSqlQuery &query, const QString &connection)
{
  if (query.isSelect())
  {
    m_currentResultsetConnection = connection;

    mainWindow()->showToolView(m_dataOutputToolView);

    m_dataOutputWidget->showQueryResultSets(query);
  }
}


void KateSQLView::slotConnectionCreated(const QString &name)
{
  m_connectionsComboBox->setCurrentItem(name);

  m_schemaBrowserWidget->schemaWidget()->buildTree(name);
}

//END KateSQLView

