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

#ifndef SQLMANAGER_H
#define SQLMANAGER_H

class ConnectionModel;
class KConfigGroup;

#include "connection.h"
#include <qsqlquery.h>

class SQLManager : public QObject
{
  Q_OBJECT

  public:
    SQLManager(QObject *parent = 0);
    ~SQLManager();

    ConnectionModel *connectionModel();
    void createConnection(const Connection &conn);
    bool testConnection(const Connection &conn, QSqlError &error);

  public slots:
    void removeConnection(const QString &name);
    void loadConnections(KConfigGroup *connectionsGroup);
    void saveConnections(KConfigGroup *connectionsGroup);
    void runQuery(const QString &text, const QString &connection );

  protected:
    void saveConnection(KConfigGroup *connectionsGroup, const Connection &conn);

  signals:
    void connectionCreated(const QString &name);
    void connectionRemoved(const QString &name);

    void queryActivated(QSqlQuery &query);

    void error(const QString &message);
    void success(const QString &message);

  private:
    ConnectionModel *m_model;
};

#endif // SQLMANAGER_H
