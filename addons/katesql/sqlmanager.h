/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef SQLMANAGER_H
#define SQLMANAGER_H

class ConnectionModel;
class KConfigGroup;
class QUrl;

#include "connection.h"
#include <KWallet>
#include <QSqlQuery>
#include <QUrl>

class SQLManager : public QObject
{
    Q_OBJECT

public:
    SQLManager(QObject *parent = nullptr);
    ~SQLManager() override;

    ConnectionModel *connectionModel();
    void createConnection(const Connection &conn);
    bool testConnection(const Connection &conn, QSqlError &error);
    bool isValidAndOpen(const QString &connection);

    KWallet::Wallet *openWallet();
    int storeCredentials(const Connection &conn);
    int readCredentials(const QString &name, QString &password);
    static const int K_WALLET_CONNECTION_SUCCESSFUL = 0;
    static const int K_WALLET_CONNECTION_ERROR = -1;
    static const int K_WALLET_CONNECTION_REJECTED_BY_USER = -2;

public Q_SLOTS:
    void removeConnection(const QString &name);
    void reopenConnection(const QString &name);
    void loadConnections(const KConfigGroup &connectionsGroup);
    void saveConnections(KConfigGroup *connectionsGroup);
    void runQuery(const QString &text, const QString &connection);

protected:
    void saveConnection(KConfigGroup *connectionsGroup, const Connection &conn);

Q_SIGNALS:
    void connectionCreated(const QString &name);
    void connectionRemoved(const QString &name);
    void connectionAboutToBeClosed(const QString &name);

    void queryActivated(QSqlQuery &query, const QString &connection);

    void error(const QString &message);
    void success(const QString &message);

private:
    ConnectionModel *m_model;
    KWallet::Wallet *m_wallet = nullptr;
};

#endif // SQLMANAGER_H
