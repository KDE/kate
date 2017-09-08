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

#include <klocalizedstring.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <QDebug>
#include <qsqldatabase.h>
#include <qsqlquery.h>
#include <qsqlerror.h>
#include <qsqldriver.h>

using KWallet::Wallet;

SQLManager::SQLManager(QObject *parent)
: QObject(parent)
, m_model(new ConnectionModel(this))
, m_wallet(nullptr)
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
    qDebug() << "connection" << conn.name << "already exist";
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

  // try to open connection, with or without password
  if (db.open())
    m_model->setStatus(conn.name, Connection::ONLINE);
  else
  {
    if (conn.status != Connection::REQUIRE_PASSWORD)
    {
      m_model->setStatus(conn.name, Connection::OFFLINE);
      emit error(db.lastError().text());
    }
  }

  emit connectionCreated(conn.name);
}


bool SQLManager::testConnection(const Connection &conn, QSqlError &error)
{
  QString connectionName = (conn.name.isEmpty()) ? QString::fromLatin1 ("katesql-test") : conn.name;

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

bool SQLManager::isValidAndOpen(const QString &connection)
{
  QSqlDatabase db = QSqlDatabase::database(connection);

  if (!db.isValid())
  {
    m_model->setStatus(connection, Connection::OFFLINE);
    emit error(db.lastError().text());
    return false;
  }

  if (!db.isOpen())
  {
    qDebug() << "database connection is not open. trying to open it...";

    if (m_model->status(connection) == Connection::REQUIRE_PASSWORD)
    {
      QString password;
      int ret = readCredentials(connection, password);

      if (ret != 0)
        qDebug() << "Can't retrieve password from kwallet. returned code" << ret;
      else
      {
        db.setPassword(password);
        m_model->setPassword(connection, password);
      }
    }

    if (!db.open())
    {
      m_model->setStatus(connection, Connection::OFFLINE);
      emit error(db.lastError().text());
      return false;
    }
  }

  m_model->setStatus(connection, Connection::ONLINE);

  return true;
}


void SQLManager::reopenConnection (const QString& name)
{
  emit connectionAboutToBeClosed(name);

  QSqlDatabase db = QSqlDatabase::database(name);

  db.close();
  isValidAndOpen(name);
}


Wallet *SQLManager::openWallet()
{
  if (!m_wallet)
    /// FIXME get kate window id...
    m_wallet = Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0);

  if (!m_wallet)
    return nullptr;

  QString folder (QLatin1String ("SQL Connections"));

  if (!m_wallet->hasFolder(folder))
    m_wallet->createFolder(folder);

  m_wallet->setFolder(folder);

  return m_wallet;
}


// return 0 on success, -1 on error, -2 on user reject
int SQLManager::storeCredentials(const Connection &conn)
{
  // Sqlite is without password, avoid to open wallet
  if (conn.driver.contains(QLatin1String ("QSQLITE")))
    return 0;

  Wallet *wallet = openWallet();

  if (!wallet) // user reject
    return -2;

  QMap<QString, QString> map;

  map[QLatin1String ("driver")] = conn.driver.toUpper();
  map[QLatin1String ("hostname")] = conn.hostname.toUpper();
  map[QLatin1String ("port")] = QString::number(conn.port);
  map[QLatin1String ("database")] = conn.database.toUpper();
  map[QLatin1String ("username")] = conn.username;
  map[QLatin1String ("password")] = conn.password;

  return (wallet->writeMap(conn.name, map) == 0) ? 0 : -1;
}


// return 0 on success, -1 on error or not found, -2 on user reject
// if success, password contain the password
int SQLManager::readCredentials(const QString &name, QString &password)
{
  Wallet *wallet = openWallet();

  if (!wallet) // user reject
    return -2;

  QMap<QString, QString> map;

  if (wallet->readMap(name, map) == 0)
  {
    if (!map.isEmpty())
    {
      password = map.value(QLatin1String("password"));
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
  emit connectionAboutToBeClosed(name);

  m_model->removeConnection(name);

  QSqlDatabase::removeDatabase(name);

  emit connectionRemoved(name);
}

/// TODO: read KUrl instead of QString for sqlite paths
void SQLManager::loadConnections(KConfigGroup *connectionsGroup)
{
  Connection c;

  foreach ( const QString& groupName, connectionsGroup->groupList() )
  {
    qDebug() << "reading group:" << groupName;

    KConfigGroup group = connectionsGroup->group(groupName);

    c.name     = groupName;
    c.driver   = group.readEntry("driver");
    c.database = group.readEntry("database");
    c.options  = group.readEntry("options");

    if (!c.driver.contains(QLatin1String("QSQLITE")))
    {
      c.hostname = group.readEntry("hostname");
      c.username = group.readEntry("username");
      c.port     = group.readEntry("port", 0);

      // for compatibility with version 0.2, when passwords
      // were stored in config file instead of kwallet
      c.password = group.readEntry("password");

      if (!c.password.isEmpty())
        c.status = Connection::ONLINE;
      else
        c.status = Connection::REQUIRE_PASSWORD;
    }
    createConnection(c);
  }
}

void SQLManager::saveConnections(KConfigGroup *connectionsGroup)
{
  for(int i = 0; i < m_model->rowCount(); i++)
    saveConnection(connectionsGroup, m_model->data(m_model->index(i), Qt::UserRole).value<Connection>());
}

/// TODO: write KUrl instead of QString for sqlite paths
void SQLManager::saveConnection(KConfigGroup *connectionsGroup, const Connection &conn)
{
  qDebug() << "saving connection" << conn.name;

  KConfigGroup group = connectionsGroup->group(conn.name);

  group.writeEntry("driver"  , conn.driver);
  group.writeEntry("database", conn.database);
  group.writeEntry("options" , conn.options);

  if (!conn.driver.contains(QLatin1String("QSQLITE")))
  {
    group.writeEntry("hostname", conn.hostname);
    group.writeEntry("username", conn.username);
    group.writeEntry("port"    , conn.port);
  }
}


void SQLManager::runQuery(const QString &text, const QString &connection)
{
  qDebug() << "connection:" << connection;
  qDebug() << "text:"       << text;

  if (text.isEmpty())
    return;

  if (!isValidAndOpen(connection))
    return;

  QSqlDatabase db = QSqlDatabase::database(connection);
  QSqlQuery query(db);

  if (!query.prepare(text))
  {
    QSqlError err = query.lastError();

    if (err.type() == QSqlError::ConnectionError)
      m_model->setStatus(connection, Connection::OFFLINE);

    emit error(err.text());
    return;
  }

  if (!query.exec())
  {
    QSqlError err = query.lastError();

    if (err.type() == QSqlError::ConnectionError)
      m_model->setStatus(connection, Connection::OFFLINE);

    emit error(err.text());
    return;
  }

  QString message;

  /// TODO: improve messages
  if (query.isSelect())
  {
    if (!query.driver()->hasFeature(QSqlDriver::QuerySize))
      message = i18nc("@info", "Query completed successfully");
    else
    {
      int nRowsSelected = query.size();
      message = i18ncp("@info", "%1 record selected", "%1 records selected", nRowsSelected);
    }
  }
  else
  {
    int nRowsAffected = query.numRowsAffected();
    message = i18ncp("@info", "%1 row affected", "%1 rows affected", nRowsAffected);
  }

  emit success(message);
  emit queryActivated(query, connection);
}

