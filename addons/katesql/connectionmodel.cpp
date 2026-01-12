/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "connectionmodel.h"

#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QSize>
#include <QtCore/qlogging.h>

ConnectionModel::ConnectionModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_icons[Connection::UNKNOWN] = QIcon::fromTheme(QStringLiteral("user-offline"));
    m_icons[Connection::ONLINE] = QIcon::fromTheme(QStringLiteral("user-online"));
    m_icons[Connection::OFFLINE] = QIcon::fromTheme(QStringLiteral("user-offline"));
    m_icons[Connection::REQUIRE_PASSWORD] = QIcon::fromTheme(QStringLiteral("user-invisible"));
}

ConnectionModel::~ConnectionModel() = default;

int ConnectionModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_connections.count();
}

QVariant ConnectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
        return m_connections.at(index.row()).name;

    case Qt::UserRole:
        return QVariant::fromValue<Connection>(m_connections.at(index.row()));

    case Qt::DecorationRole:
        return m_icons[m_connections.at(index.row()).status];

    case Qt::SizeHintRole: {
        const QFontMetrics metrics(QFontDatabase::systemFont(QFontDatabase::GeneralFont));

        return QSize(metrics.boundingRect(m_connections.at(index.row()).name).width(), 22);
    }

    default:
        return {};
    }

    return {};
}

int ConnectionModel::addConnection(const Connection &conn)
{
    if (indexOf(conn.name) != -1) {
        qDebug("a connection named %ls already exists!", qUtf16Printable(conn.name));
        return -1;
    }

    int pos = m_connections.count();

    beginInsertRows(QModelIndex(), pos, pos);

    m_connections.push_back(conn);

    endInsertRows();

    return pos;
}

void ConnectionModel::removeConnection(const QString &name)
{
    int pos = indexOf(name);

    beginRemoveRows(QModelIndex(), pos, pos);

    m_connections.remove(pos);

    endRemoveRows();
}

int ConnectionModel::indexOf(const QString &name) const
{
    auto it = std::find_if(m_connections.cbegin(), m_connections.cend(), [name](const Connection &c) {
        return c.name == name;
    });
    if (it != m_connections.cend()) {
        return static_cast<int>(std::distance(m_connections.cbegin(), it));
    }
    return -1;
}

Connection::Status ConnectionModel::status(const QString &name) const
{
    const int pos = indexOf(name);
    if (pos == -1) {
        return Connection::UNKNOWN;
    }

    return m_connections[pos].status;
}

void ConnectionModel::setStatus(const QString &name, const Connection::Status status)
{
    const int pos = indexOf(name);
    if (pos == -1) {
        return;
    }

    m_connections[pos].status = status;

    Q_EMIT dataChanged(index(pos), index(pos));
}

void ConnectionModel::setPassword(const QString &name, const QString &password)
{
    const int pos = indexOf(name);
    if (pos == -1) {
        return;
    }

    m_connections[pos].password = password;

    Q_EMIT dataChanged(index(pos), index(pos));
}
