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

#include "connectionwizard.h"
#include "sqlmanager.h"

#include <klocale.h>
#include <kcombobox.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <kurlrequester.h>
#include <kdebug.h>
#include <knuminput.h>

#include <qformlayout.h>
#include <qsqldatabase.h>
#include <qsqlerror.h>

ConnectionWizard::ConnectionWizard ( SQLManager *manager, Connection *conn, QWidget *parent, Qt::WindowFlags flags )
: QWizard(parent, flags)
, m_manager(manager)
, m_connection(conn)
{
  setWindowTitle(i18nc("@title:window", "Connection Wizard"));

  setPage(Page_Driver, new ConnectionDriverPage);
  setPage(Page_Standard_Server, new ConnectionStandardServerPage);
  setPage(Page_SQLite_Server, new ConnectionSQLiteServerPage);
  setPage(Page_Save, new ConnectionSavePage);
}

ConnectionWizard::~ConnectionWizard()
{
}

ConnectionDriverPage::ConnectionDriverPage ( QWidget *parent)
: QWizardPage(parent)
{
  setTitle(i18nc("@title Wizard page title", "Database Driver"));
  setSubTitle(i18nc("@title Wizard page subtitle", "Select the database driver"));

  QFormLayout *layout = new QFormLayout();

  driverComboBox = new KComboBox();
  driverComboBox->addItems(QSqlDatabase::drivers());

  layout->addRow(i18nc("@label:listbox", "Database driver:"),   driverComboBox);

  setLayout(layout);

  registerField("driver", driverComboBox, "currentText");
}


void ConnectionDriverPage::initializePage()
{
  ConnectionWizard *wiz = static_cast<ConnectionWizard*>(wizard());
  Connection *c = wiz->connection();

  if (!c->driver.isEmpty())
    driverComboBox->setCurrentItem(c->driver);
}

int ConnectionDriverPage::nextId() const
{
  if (driverComboBox->currentText().contains("QSQLITE"))
      return ConnectionWizard::Page_SQLite_Server;
  else
    return ConnectionWizard::Page_Standard_Server;
}

ConnectionStandardServerPage::ConnectionStandardServerPage ( QWidget *parent )
: QWizardPage(parent)
{
  setTitle(i18nc("@title Wizard page title", "Connection Parameters"));
  setSubTitle(i18nc("@title Wizard page subtitle", "Please enter connection parameters"));

  QFormLayout *layout = new QFormLayout();

  hostnameLineEdit = new KLineEdit();
  usernameLineEdit = new KLineEdit();
  passwordLineEdit = new KLineEdit();
  databaseLineEdit = new KLineEdit();
  optionsLineEdit  = new KLineEdit();
  portSpinBox      = new KIntSpinBox();

  portSpinBox->setMaximum(65535);
  portSpinBox->setSpecialValueText(i18nc("@item Spinbox special value", "Default"));
  portSpinBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

  passwordLineEdit->setPasswordMode(true);

  layout->addRow(i18nc("@label:textbox", "Hostname:"), hostnameLineEdit);
  layout->addRow(i18nc("@label:textbox", "Username:"), usernameLineEdit);
  layout->addRow(i18nc("@label:textbox", "Password:"), passwordLineEdit);
  layout->addRow(i18nc("@label:spinbox", "Port:"), portSpinBox);
  layout->addRow(i18nc("@label:textbox", "Database name:"), databaseLineEdit);
  layout->addRow(i18nc("@label:textbox", "Connection options:"), optionsLineEdit);

  setLayout(layout);

  registerField("hostname*", hostnameLineEdit);
  registerField("username" , usernameLineEdit);
  registerField("password" , passwordLineEdit);
  registerField("database" , databaseLineEdit);
  registerField("stdOptions" , optionsLineEdit);
  registerField("port"     , portSpinBox);
}


ConnectionStandardServerPage::~ConnectionStandardServerPage()
{
}


void ConnectionStandardServerPage::initializePage()
{
  ConnectionWizard *wiz = static_cast<ConnectionWizard*>(wizard());
  Connection *c = wiz->connection();

  hostnameLineEdit->setText("localhost");

  if (c->driver == field("driver").toString())
  {
    hostnameLineEdit->setText(c->hostname);
    usernameLineEdit->setText(c->username);
    passwordLineEdit->setText(c->password);
    databaseLineEdit->setText(c->database);
    optionsLineEdit->setText(c->options);
    portSpinBox->setValue(c->port);
  }

  hostnameLineEdit->selectAll();
}


bool ConnectionStandardServerPage::validatePage()
{
  Connection c;

  c.driver   = field("driver").toString();
  c.hostname = field("hostname").toString();
  c.username = field("username").toString();
  c.password = field("password").toString();
  c.database = field("database").toString();
  c.options  = field("stdOptions").toString();
  c.port     = field("port").toInt();

  QSqlError e;

  ConnectionWizard *wiz = static_cast<ConnectionWizard*>(wizard());

  if (!wiz->manager()->testConnection(c, e))
  {
    KMessageBox::error(this, i18n("Unable to connect to database.") + "\n" + e.text());
    return false;
  }

  return true;
}

int ConnectionStandardServerPage::nextId() const
{
  return ConnectionWizard::Page_Save;
}

ConnectionSQLiteServerPage::ConnectionSQLiteServerPage ( QWidget *parent)
: QWizardPage(parent)
{
  setTitle(i18nc("@title Wizard page title", "Connection Parameters"));
  setSubTitle(i18nc("@title Wizard page subtitle", "Please enter the SQLite database file path.\nIf the file does not exist, a new database will be created."));

  QFormLayout *layout = new QFormLayout();

  pathUrlRequester = new KUrlRequester(this);
  optionsLineEdit  = new KLineEdit();

  pathUrlRequester->setMode( KFile::File | KFile::ExistingOnly | KFile::LocalOnly);
  pathUrlRequester->setFilter("*.db *.sqlite|" + i18n("Database files") + "\n*|" + i18n("All files"));

  layout->addRow(i18nc("@label:textbox", "Path:"), pathUrlRequester);
  layout->addRow(i18nc("@label:textbox", "Connection options:"), optionsLineEdit);

  setLayout(layout);

  registerField("path*", pathUrlRequester->lineEdit());
  registerField("sqliteOptions", optionsLineEdit);
}


void ConnectionSQLiteServerPage::initializePage()
{
  ConnectionWizard *wiz = static_cast<ConnectionWizard*>(wizard());
  Connection *c = wiz->connection();

  if (c->driver == field("driver").toString())
  {
    pathUrlRequester->lineEdit()->setText(c->database);
    optionsLineEdit->setText(c->options);
  }
}

bool ConnectionSQLiteServerPage::validatePage()
{
  Connection c;

  c.driver   = field("driver").toString();
  c.database = field("path").toString();
  c.options  = field("sqliteOptions").toString();

  QSqlError e;

  ConnectionWizard *wiz = static_cast<ConnectionWizard*>(wizard());

  if (!wiz->manager()->testConnection(c, e))
  {
    KMessageBox::error(this, i18nc("@info", "Unable to connect to database.<nl/><message>%1</message>").arg(e.text()));
    return false;
  }

  return true;
}

int ConnectionSQLiteServerPage::nextId() const
{
  return ConnectionWizard::Page_Save;
}

ConnectionSavePage::ConnectionSavePage ( QWidget *parent)
: QWizardPage(parent)
{
  setTitle(i18nc("@title Wizard page title", "Connection Name"));
  setSubTitle(i18nc("@title Wizard page subtitle", "Enter a unique connection name"));

  QFormLayout *layout = new QFormLayout();

  connectionNameLineEdit = new KLineEdit();

  layout->addRow(i18nc("@label:textbox", "Connection name:"), connectionNameLineEdit);

  setLayout(layout);

  registerField("connectionName*", connectionNameLineEdit);
}

void ConnectionSavePage::initializePage()
{
  QString name;

  ConnectionWizard *wiz = static_cast<ConnectionWizard*>(wizard());
  Connection *c = wiz->connection();

  if (!c->name.isEmpty())
  {
    name = c->name;
  }
  else if (field("driver").toString().contains("QSQLITE"))
  {
    /// TODO: use db file basename
    name = "SQLite";

    for (int i = 1; QSqlDatabase::contains(name); i++)
      name = QString("%1%2").arg("SQLite").arg(i);
  }
  else
  {
    name = QString("%1 on %2").arg(field("database").toString()).arg(field("hostname").toString()).simplified();

    for (int i = 1; QSqlDatabase::contains(name); i++)
      name = QString("%1 on %2 (%3)").arg(field("database").toString()).arg(field("hostname").toString()).arg(i).simplified();
  }

  connectionNameLineEdit->setText(name);
  connectionNameLineEdit->selectAll();
}


bool ConnectionSavePage::validatePage()
{
  QString name = field("connectionName").toString().simplified();

  ConnectionWizard *wiz = static_cast<ConnectionWizard*>(wizard());
  Connection *c = wiz->connection();

  c->name   = name;
  c->driver = field("driver").toString();

  if (field("driver").toString().contains("QSQLITE"))
  {
    c->database = field("path").toString();
    c->options  = field("sqliteOptions").toString();
  }
  else
  {
    c->hostname = field("hostname").toString();
    c->username = field("username").toString();
    c->password = field("password").toString();
    c->database = field("database").toString();
    c->options  = field("stdOptions").toString();
    c->port     = field("port").toInt();
  }

  return true;
}

int ConnectionSavePage::nextId() const
{
  return -1;
}
