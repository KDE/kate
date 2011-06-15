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

#include "sqlmanager.h"
#include "connectionmodel.h"

#include <kpassworddialog.h>
#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <qsqldatabase.h>
#include <qsqlquery.h>
#include <qsqlerror.h>
#include <qsqldriver.h>

using KWallet::Wallet;

SQLManager::SQLManager(QObject *parent)
: QObject(parent)
, m_model(new ConnectionModel(this))
, m_wallet(0)
{
}


SQLManager::~SQLManager()
{
  for(int i = 0; i < m_model->rowCount(); i++)
  {
    QString connection =m_model->data(m_model->index(i), Qt::DisplayRole).toString();
    QSqlDatabase::removeDatabase(connection);
  }

  delete m_model;
  delete m_wallet;
}


void SQLManager::createConnection(const Connection &conn)
{
  if (QSqlDatabase::contains(conn.name))
  {
    kDebug() << "connection" << conn.name << "already exist";
    QSqlDatabase::removeDatabase(conn.name);
  }

  QSqlDatabase db = QSqlDatabase::addDatabase(conn.driver, conn.name);

  if (!db.isValid())
  {
    emit error(db.lastError().text());
    QSqlDatabase::removeDatabase(conn.name);
    return;
  }

  db.setHostName(conn.hostname);
  db.setUserName(conn.username);
  db.setPassword(conn.password);
  db.setDatabaseName(conn.database);
  db.setConnectOptions(conn.options);

  if (conn.port > 0)
    db.setPort(conn.port);

  m_model->addConnection(conn);

  if (!db.open())
  {
    emit error(db.lastError().text());

    m_model->setEnabled(conn.name, false);
//     conn.enabled = false;
//    QSqlDatabase::removeDatabase(conn.name);
//    return;
  }
  else
    m_model->setEnabled(conn.name, true);

  emit connectionCreated(conn.name);
}


bool SQLManager::testConnection(const Connection &conn, QSqlError &error)
{
  QString connectionName = (conn.name.isEmpty()) ? "katesql-test" : conn.name;

  QSqlDatabase db = QSqlDatabase::addDatabase(conn.driver, connectionName);

  if (!db.isValid())
  {
    error = db.lastError();
    QSqlDatabase::removeDatabase(connectionName);
    return false;
  }

  db.setHostName(conn.hostname);
  db.setUserName(conn.username);
  db.setPassword(conn.password);
  db.setDatabaseName(conn.database);
  db.setConnectOptions(conn.options);

  if (conn.port > 0)
    db.setPort(conn.port);

  if (!db.open())
  {
    error = db.lastError();
    QSqlDatabase::removeDatabase(connectionName);
    return false;
  }

  QSqlDatabase::removeDatabase(connectionName);
  return true;
}


Wallet *SQLManager::openWallet()
{
  if (!m_wallet)
    /// FIXME get kate window id...
    m_wallet = Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0);

  if (!m_wallet)
    return 0;

  QString folder("SQL Connections");

  if (!m_wallet->hasFolder(folder))
    m_wallet->createFolder(folder);

  m_wallet->setFolder(folder);

  return m_wallet;
}


// return 0 on success, -1 on error, -2 on user reject
int SQLManager::storeCredentials(const Connection &conn)
{
  Wallet *wallet = openWallet();

  if (!wallet) // user reject
    return -2;

  QMap<QString, QString> map;

  map["driver"] = conn.driver.toUpper();
  map["hostname"] = conn.hostname.toUpper();
  map["port"] = QString::number(conn.port);
  map["database"] = conn.database.toUpper();
  map["username"] = conn.username;
  map["password"] = conn.password;

  return (wallet->writeMap(conn.name, map) == 0) ? 0 : -1;
}


// return 0 on success, -1 on error or not found, -2 on user reject
// if success, conn.password contain the password
int SQLManager::readCredentials(Connection &conn)
{
  Wallet *wallet = openWallet();

  if (!wallet) // user reject
    return -2;

  QMap<QString, QString> map;

  if (wallet->readMap(conn.name, map) == 0)
  {
    if (!map.isEmpty())
    {
      conn.password = map.value("password");
      return 0;
    }
  }

  return -1;
}


ConnectionModel* SQLManager::connectionModel()
{
  return m_model;
}


void SQLManager::removeConnection(const QString &name)
{
  m_model->removeConnection(name);

  QSqlDatabase::removeDatabase(name);

  emit connectionRemoved(name);
}

/// TODO: read KUrl instead of QString for sqlite paths
void SQLManager::loadConnections(KConfigGroup *connectionsGroup)
{
  Connection c;

  Wallet *wallet = 0;
  bool walletErrorOrUserReject = false;

  foreach ( const QString& groupName, connectionsGroup->groupList() )
  {
    kDebug() << "reading group:" << groupName;

    KConfigGroup group = connectionsGroup->group(groupName);

    c.name     = groupName;
    c.driver   = group.readEntry("driver");
    c.database = group.readEntry("database");
    c.options  = group.readEntry("options");

    if (!c.driver.contains("QSQLITE"))
    {
      c.hostname = group.readEntry("hostname");
      c.username = group.readEntry("username");
      c.port     = group.readEntry("port", 0);

      // for compatibility with version 0.2, when passwords
      // were stored in config file instead of kwallet
      c.password = group.readEntry("password");

      if (!c.password.isEmpty())
        continue;

      if (!walletErrorOrUserReject)
      {
        wallet = openWallet();

        if (!wallet)
          walletErrorOrUserReject = true;
        else
        {
          int ret = readCredentials(c);

          if (ret != 0)
            kDebug() << "Can't retrieve password from kwallet. returned code" << ret;
        }
      }
    }
    createConnection(c);
  }
}

void SQLManager::saveConnections(KConfigGroup *connectionsGroup)
{
  for(int i = 0; i < m_model->rowCount(); i++)
    saveConnection(connectionsGroup, qVariantValue<Connection>(m_model->data(m_model->index(i), Qt::UserRole)));
}

/// TODO: write KUrl instead of QString for sqlite paths
void SQLManager::saveConnection(KConfigGroup *connectionsGroup, const Connection &conn)
{
  kDebug() << "saving connection" << conn.name;

  KConfigGroup group = connectionsGroup->group(conn.name);

  group.writeEntry("driver"  , conn.driver);
  group.writeEntry("database", conn.database);
  group.writeEntry("options" , conn.options);

  if (!conn.driver.contains("QSQLITE"))
  {
    group.writeEntry("hostname", conn.hostname);
    group.writeEntry("username", conn.username);
    group.writeEntry("port"    , conn.port);
  }
}


void SQLManager::runQuery(const QString &text, const QString &connection)
{
  kDebug() << "connection:" << connection;
  kDebug() << "text:"       << text;

  if (text.isEmpty())
    return;

  QSqlDatabase db = QSqlDatabase::database(connection);

  if (!db.isValid())
  {
    m_model->setEnabled(connection, false);
    emit error(db.lastError().text());
    return;
  }

  if (!db.isOpen())
  {
    kDebug() << "database connection is not open. trying to open it...";

    if (!db.open())
    {
      m_model->setEnabled(connection, false);
      emit error(db.lastError().text());
      return;
    }
  }

  m_model->setEnabled(connection, true);

  QSqlQuery query(db);

  if (!query.prepare(text))
  {
    emit error(query.lastError().text());
    return;
  }

  if (!query.exec())
  {
    emit error(query.lastError().text());
    return;
  }

  QString message;

  /// TODO: improve messages
  if (query.isSelect())
  {
    if (!query.driver()->hasFeature(QSqlDriver::QuerySize))
      message = i18nc("@info/plain", "Query completed successfully");
    else
    {
      int nRowsSelected = query.size();
      message = i18ncp("@info/plain", "%1 record selected", "%1 records selected", nRowsSelected);
    }
  }
  else
  {
    int nRowsAffected = query.numRowsAffected();
    message = i18ncp("@info/plain", "%1 row affected", "%1 rows affected", nRowsAffected);
  }

  emit success(message);
  emit queryActivated(query, connection);
}

