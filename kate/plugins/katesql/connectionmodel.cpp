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

#include <kdebug.h>
#include <kicon.h>
#include <kglobalsettings.h>

#include <qfontmetrics.h>
#include <qfont.h>
#include <qsize.h>
#include <qstringlist.h>
#include <qvariant.h>

ConnectionModel::ConnectionModel(QObject *parent)
: QAbstractListModel(parent)
{
  enabledIcon  = KIcon("user-online");
  disabledIcon = KIcon("user-offline");
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
      return (m_connections.value(key).enabled) ? enabledIcon : disabledIcon;
    break;

    case Qt::SizeHintRole:
    {
      const QFontMetrics metrics(KGlobalSettings::generalFont());
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
    kWarning() << "a connection named" << conn.name << "already exists!";
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


void ConnectionModel::setEnabled(const QString &name, bool enabled)
{
  if (!m_connections.contains(name))
    return;

  m_connections[name].enabled = enabled;

  const int i = indexOf(name);

  emit dataChanged(index(i), index(i));
}
