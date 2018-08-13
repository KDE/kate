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
#include "outputwidget.h"

#include <ktexteditor/plugin.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/application.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QAction>
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kcombobox.h>
#include <KSharedConfig>
#include <KXMLGUIFactory>

#include <qmenu.h>
#include <qstring.h>
#include <qsqlquery.h>
#include <QVBoxLayout>
#include <QApplication>
#include <QWidgetAction>

KateSQLView::KateSQLView(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mw)
: QObject (mw)
, KXMLGUIClient()
, m_manager (new SQLManager(this))
, m_mainWindow (mw)
{
  KXMLGUIClient::setComponentName (QLatin1String("katesql"), i18n ("Kate SQL Plugin"));
  setXMLFile( QLatin1String("ui.rc") );

  m_outputToolView    = mw->createToolView(plugin, QLatin1String ("kate_private_plugin_katesql_output"),
                                               KTextEditor::MainWindow::Bottom,
                                               QIcon::fromTheme (QLatin1String ("view-form-table")),
                                               i18nc("@title:window", "SQL Results")
                                               );

  m_schemaBrowserToolView = mw->createToolView(plugin, QLatin1String ("kate_private_plugin_katesql_schemabrowser"),
                                               KTextEditor::MainWindow::Left,
                                               QIcon::fromTheme (QLatin1String ("view-list-tree")),
                                               i18nc("@title:window", "SQL Schema Browser")
                                               );

  m_outputWidget = new KateSQLOutputWidget(m_outputToolView);

  m_schemaBrowserWidget = new SchemaBrowserWidget(m_schemaBrowserToolView, m_manager);

  m_connectionsComboBox = new KComboBox(false);
  m_connectionsComboBox->setEditable(false);
  m_connectionsComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
  m_connectionsComboBox->setModel(m_manager->connectionModel());

  setupActions();

  m_mainWindow->guiFactory()->addClient(this);

  QMenu *sqlMenu = (QMenu*)factory()->container(QLatin1String ("SQL"), this);

  m_connectionsGroup = new QActionGroup(sqlMenu);
  m_connectionsGroup->setExclusive(true);

  connect(sqlMenu, &QMenu::aboutToShow, this, &KateSQLView::slotSQLMenuAboutToShow);
  connect(m_connectionsGroup, &QActionGroup::triggered, this, &KateSQLView::slotConnectionSelectedFromMenu);
  connect(m_manager, &SQLManager::error, this, &KateSQLView::slotError);
  connect(m_manager, &SQLManager::success, this, &KateSQLView::slotSuccess);
  connect(m_manager, &SQLManager::queryActivated, this, &KateSQLView::slotQueryActivated);
  connect(m_manager, &SQLManager::connectionCreated, this, &KateSQLView::slotConnectionCreated);
  connect(m_manager, &SQLManager::connectionAboutToBeClosed, this, &KateSQLView::slotConnectionAboutToBeClosed);
  connect(m_connectionsComboBox, static_cast<void (KComboBox::*)(const QString &)>(&KComboBox::currentIndexChanged), this, &KateSQLView::slotConnectionChanged);

  stateChanged(QLatin1String ("has_connection_selected"), KXMLGUIClient::StateReverse);
}


KateSQLView::~KateSQLView()
{
  m_mainWindow->guiFactory()->removeClient( this );

  delete m_outputToolView;
  delete m_schemaBrowserToolView;

  delete m_manager;
}


void KateSQLView::setupActions()
{
  QAction* action;
  KActionCollection* collection = actionCollection();

  action = collection->addAction(QLatin1String ("connection_create"));
  action->setText( i18nc("@action:inmenu", "Add connection...") );
  action->setIcon( QIcon::fromTheme (QLatin1String ("list-add")) );
  connect(action, &QAction::triggered, this, &KateSQLView::slotConnectionCreate);

  action = collection->addAction(QLatin1String ("connection_remove"));
  action->setText( i18nc("@action:inmenu", "Remove connection") );
  action->setIcon( QIcon::fromTheme (QLatin1String ("list-remove")) );
  connect(action, &QAction::triggered, this, &KateSQLView::slotConnectionRemove);

  action = collection->addAction(QLatin1String ("connection_edit"));
  action->setText( i18nc("@action:inmenu", "Edit connection...") );
  action->setIcon( QIcon::fromTheme (QLatin1String ("configure")) );
  connect(action, &QAction::triggered, this, &KateSQLView::slotConnectionEdit);

  action = collection->addAction(QLatin1String ("connection_reconnect"));
  action->setText( i18nc("@action:inmenu", "Reconnect") );
  action->setIcon(  QIcon::fromTheme (QLatin1String ("view-refresh")) );
  connect(action, &QAction::triggered, this, &KateSQLView::slotConnectionReconnect);

  QWidgetAction *wa = new QWidgetAction(this);
  collection->addAction(QLatin1String ("connection_chooser"), wa);
  wa->setText( i18nc("@action:intoolbar", "Connection") );
  wa->setDefaultWidget(m_connectionsComboBox);

  action = collection->addAction(QLatin1String ("query_run"));
  action->setText( i18nc("@action:inmenu", "Run query") );
  action->setIcon( QIcon::fromTheme (QLatin1String ("quickopen")) );
  collection->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_E) );
  connect(action, &QAction::triggered, this, &KateSQLView::slotRunQuery);

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

  QMenu *sqlMenu = (QMenu*)factory()->container(QLatin1String ("SQL"), this);
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
  stateChanged(QLatin1String ("has_connection_selected"), (connection.isEmpty()) ? KXMLGUIClient::StateReverse : KXMLGUIClient::StateNoReverse);

  m_schemaBrowserWidget->schemaWidget()->buildTree(connection);
}


void KateSQLView::slotGlobalSettingsChanged()
{
  m_outputWidget->dataOutputWidget()->model()->readConfig();
}


void KateSQLView::readSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
  KConfigGroup globalConfig(KSharedConfig::openConfig(), "KateSQLPlugin");

  bool saveConnections = globalConfig.readEntry("SaveConnections", true);

  if (!saveConnections)
    return;

  KConfigGroup group(config, groupPrefix + QLatin1String (":connections"));

  m_manager->loadConnections(&group);

  QString lastConnection = group.readEntry("LastUsed");

  if (m_connectionsComboBox->contains(lastConnection))
    m_connectionsComboBox->setCurrentItem(lastConnection);
}


void KateSQLView::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
  KConfigGroup group(config, groupPrefix + QLatin1String (":connections"));

  group.deleteGroup();

  KConfigGroup globalConfig(KSharedConfig::openConfig(), "KateSQLPlugin");
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
    c.name = QString::fromLatin1("%1 (%2)").arg(c.name).arg(i);

  m_manager->createConnection(c);

  if (m_manager->storeCredentials(c) != 0)
    qDebug() << "Connection credentials not saved";
}


void KateSQLView::slotConnectionEdit()
{
  int i = m_connectionsComboBox->currentIndex();

  if (i == -1)
    return;

  ConnectionModel *model = m_manager->connectionModel();
  Connection c = model->data(model->index(i), Qt::UserRole).value<Connection>();

  QString previousName = c.name;

  ConnectionWizard wizard(m_manager, &c);

  if (wizard.exec() != QDialog::Accepted)
    return;

  m_manager->removeConnection(previousName);
  m_manager->createConnection(c);

  if (m_manager->storeCredentials(c) != 0)
    qDebug() << "Connection credentials not saved";
}


void KateSQLView::slotConnectionRemove()
{
  QString connection = m_connectionsComboBox->currentText();

  if (!connection.isEmpty())
    m_manager->removeConnection(connection);
}


void KateSQLView::slotConnectionReconnect()
{
  QString connection = m_connectionsComboBox->currentText();

  if (!connection.isEmpty())
    m_manager->reopenConnection(connection);
}


void KateSQLView::slotConnectionAboutToBeClosed (const QString& name)
{
  /// must delete the QSqlQuery object inside the model before closing connection

  if (name == m_currentResultsetConnection)
    m_outputWidget->dataOutputWidget()->clearResults();
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

  KTextEditor::View *view = m_mainWindow->activeView();

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
  m_outputWidget->textOutputWidget()->showErrorMessage(message);
  m_outputWidget->setCurrentWidget(m_outputWidget->textOutputWidget());
  m_mainWindow->showToolView(m_outputToolView);
  
}


void KateSQLView::slotSuccess(const QString &message)
{
  m_outputWidget->textOutputWidget()->showSuccessMessage(message);
  m_outputWidget->setCurrentWidget(m_outputWidget->textOutputWidget());
  m_mainWindow->showToolView(m_outputToolView);

}


void KateSQLView::slotQueryActivated(QSqlQuery &query, const QString &connection)
{
  if (query.isSelect())
  {
    m_currentResultsetConnection = connection;

    m_outputWidget->dataOutputWidget()->showQueryResultSets(query);
    m_outputWidget->setCurrentWidget(m_outputWidget->dataOutputWidget());
    m_mainWindow->showToolView(m_outputToolView);
    
  }
}


void KateSQLView::slotConnectionCreated(const QString &name)
{
  m_connectionsComboBox->setCurrentItem(name);

  m_schemaBrowserWidget->schemaWidget()->buildTree(name);
}

//END KateSQLView

