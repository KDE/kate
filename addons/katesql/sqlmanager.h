/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

class ConnectionModel;
class KConfigGroup;
class DataOutputEditableModel;
class DataOutputModelInterface;

#include "connection.h"
#include "helpers/foreignkeyhelper.h"

#include <QMap>
#include <QSqlQuery>
#include <QUrl>

class SQLManager : public QObject
{
    Q_OBJECT

public:
    explicit SQLManager(QObject *parent = nullptr);
    ~SQLManager() override;

    ConnectionModel *connectionModel();
    void createConnection(const Connection &conn);
    static bool testConnection(const Connection &conn, QSqlError &error);
    bool isValidAndOpen(const QString &connection);

    int storeCredentials(const Connection &conn);
    int readCredentials(const QString &name, QString &password);

public Q_SLOTS:
    void removeConnection(const QString &name);
    void reopenConnection(const QString &name);
    void loadConnections(const KConfigGroup &connectionsGroup);
    void saveConnections(KConfigGroup *connectionsGroup);
    void runQuery(const QString &text, const QString &connection);
    void runEditableQuery(const QString &tableName, const QString &connection, const QMap<QString, QString> &displayColumns);
    void runEditableRelationalQuery(const QString &tableName,
                                    const QString &connection,
                                    const DatabaseForeignKeys &foreignKeys,
                                    const QMap<QString, QString> &displayColumns);

protected:
    inline static constexpr QLatin1String KeychainService = QLatin1String("org.kde.kate.katesql");

    static void saveConnection(KConfigGroup *connectionsGroup, const Connection &conn);

Q_SIGNALS:
    void connectionCreated(const QString &name);
    void connectionRemoved(const QString &name);
    void connectionAboutToBeClosed(const QString &name);

    void queryActivated(QSqlQuery &query, const QString &connection);
    void editableQueryActivated(DataOutputEditableModel *model, const QString &connection, const QMap<QString, QString> &displayColumns);
    void editableRelationalQueryActivated(DataOutputModelInterface *model, const QString &connection, const QMap<QString, QString> &displayColumns);

    void error(const QString &message);
    void success(const QString &message);

private:
    ConnectionModel *m_model;
};
