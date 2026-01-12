/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "connection.h"

#include <QAbstractListModel>
#include <QHash>
#include <QIcon>
#include <QString>

class ConnectionModel : public QAbstractListModel
{
public:
    explicit ConnectionModel(QObject *parent = nullptr);
    ~ConnectionModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    virtual int addConnection(const Connection &conn);
    virtual void removeConnection(const QString &name);

    Connection::Status status(const QString &name) const;
    void setStatus(const QString &name, const Connection::Status status);

    void setPassword(const QString &name, const QString &password);

    [[nodiscard]] int indexOf(const QString &name) const;

    //     virtual bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
    //     virtual bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());

private:
    QList<Connection> m_connections;
    QHash<Connection::Status, QIcon> m_icons;
};
