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

#ifndef CONNECTIONMODEL_H
#define CONNECTIONMODEL_H

#include "connection.h"

#include <QIcon>
#include <QAbstractListModel>
#include <qstring.h>
#include <qhash.h>

class ConnectionModel : public QAbstractListModel
{
  Q_OBJECT

  public:
    ConnectionModel(QObject *parent = nullptr);
    ~ConnectionModel() override;

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const override;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

    virtual int addConnection(Connection conn);
    virtual void removeConnection(const QString &name);

    Connection::Status status(const QString &name) const;
    void setStatus(const QString &name, const Connection::Status status);

    void setPassword(const QString &name, const QString &password);

    int indexOf(const QString &name);

//     virtual bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
//     virtual bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());

  private:
    QHash<QString, Connection> m_connections;
    QHash<Connection::Status, QIcon> m_icons;
};

#endif // CONNECTIONMODEL_H
