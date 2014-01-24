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

#include "connectionmodel.h"

#include <QDebug>
#include <QFontDatabase>
#include <QIcon>
#include <qfontmetrics.h>
#include <qfont.h>
#include <qsize.h>
#include <qstringlist.h>
#include <qvariant.h>

ConnectionModel::ConnectionModel(QObject *parent)
: QAbstractListModel(parent)
{
  m_icons[Connection::UNKNOWN]          = QIcon::fromTheme(QLatin1String("user-offline"));
  m_icons[Connection::ONLINE]           = QIcon::fromTheme(QLatin1String("user-online"));
  m_icons[Connection::OFFLINE]          = QIcon::fromTheme(QLatin1String("user-offline"));
  m_icons[Connection::REQUIRE_PASSWORD] = QIcon::fromTheme(QLatin1String("user-invisible"));
}

ConnectionModel::~ConnectionModel()
{
}

int ConnectionModel::rowCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);

  return m_connections.count();
}

QVariant ConnectionModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  const QString key = m_connections.keys().at(index.row());

  switch (role)
  {
    case Qt::DisplayRole:
      return QVariant(m_connections.value(key).name);
    break;

    case Qt::UserRole:
      return qVariantFromValue<Connection>(m_connections.value(key));
    break;

    case Qt::DecorationRole:
      return m_icons[m_connections.value(key).status];
    break;

    case Qt::SizeHintRole:
    {
      const QFontMetrics metrics(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
      return QSize(metrics.width(m_connections.value(key).name), 22);
    }
    break;

    default:
      return QVariant();
    break;
  }

  return QVariant();
}

int ConnectionModel::addConnection( Connection conn )
{
  /// FIXME
  if (m_connections.contains(conn.name))
  {
    qDebug() << "a connection named" << conn.name << "already exists!";
    return -1;
  }

  int pos = m_connections.count();

  beginInsertRows(QModelIndex(), pos, pos);

  m_connections[conn.name] = conn;

  endInsertRows();

  return m_connections.keys().indexOf(conn.name);
}

void ConnectionModel::removeConnection( const QString &name )
{
  int pos = m_connections.keys().indexOf(name);

  beginRemoveRows(QModelIndex(), pos, pos);

  m_connections.remove(name);

  endRemoveRows();
}

int ConnectionModel::indexOf(const QString &name)
{
  return m_connections.keys().indexOf(name);
}

Connection::Status ConnectionModel::status(const QString &name) const
{
  if (!m_connections.contains(name))
    return Connection::UNKNOWN;

  return m_connections[name].status;
}

void ConnectionModel::setStatus(const QString &name, const Connection::Status status)
{
  if (!m_connections.contains(name))
    return;

  m_connections[name].status = status;

  const int i = indexOf(name);

  emit dataChanged(index(i), index(i));
}

void ConnectionModel::setPassword(const QString &name, const QString &password)
{
  if (!m_connections.contains(name))
    return;

  m_connections[name].password = password;

  const int i = indexOf(name);

  emit dataChanged(index(i), index(i));
}

