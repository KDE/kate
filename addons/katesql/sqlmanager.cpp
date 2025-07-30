/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "sqlmanager.h"
#include "connectionmodel.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QApplication>
#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>

#include <qt6keychain/keychain.h>

SQLManager::SQLManager(QObject *parent)
    : QObject(parent)
    , m_model(new ConnectionModel(this))
{
}

SQLManager::~SQLManager()
{
    for (int i = 0; i < m_model->rowCount(); i++) {
        QString connection = m_model->data(m_model->index(i), Qt::DisplayRole).toString();
        QSqlDatabase::removeDatabase(connection);
    }

    delete m_model;
}

void SQLManager::createConnection(const Connection &conn)
{
    if (QSqlDatabase::contains(conn.name)) {
        qDebug("connection %ls already exist", qUtf16Printable(conn.name));
        QSqlDatabase::removeDatabase(conn.name);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(conn.driver, conn.name);

    if (!db.isValid()) {
        Q_EMIT error(db.lastError().text());
        QSqlDatabase::removeDatabase(conn.name);
        return;
    }

    db.setHostName(conn.hostname);
    db.setUserName(conn.username);
    db.setPassword(conn.password);
    db.setDatabaseName(conn.database);
    db.setConnectOptions(conn.options);

    if (conn.port > 0) {
        db.setPort(conn.port);
    }

    m_model->addConnection(conn);

    // try to open connection, with or without password
    if (db.open()) {
        m_model->setStatus(conn.name, Connection::ONLINE);
    } else {
        if (conn.status != Connection::REQUIRE_PASSWORD) {
            m_model->setStatus(conn.name, Connection::OFFLINE);
            Q_EMIT error(db.lastError().text());
        }
    }

    Q_EMIT connectionCreated(conn.name);
}

bool SQLManager::testConnection(const Connection &conn, QSqlError &error)
{
    QString connectionName = (conn.name.isEmpty()) ? QStringLiteral("katesql-test") : conn.name;

    QSqlDatabase db = QSqlDatabase::addDatabase(conn.driver, connectionName);

    if (!db.isValid()) {
        error = db.lastError();
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }

    db.setHostName(conn.hostname);
    db.setUserName(conn.username);
    db.setPassword(conn.password);
    db.setDatabaseName(conn.database);
    db.setConnectOptions(conn.options);

    if (conn.port > 0) {
        db.setPort(conn.port);
    }

    if (!db.open()) {
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

    if (!db.isValid()) {
        m_model->setStatus(connection, Connection::OFFLINE);
        Q_EMIT error(db.lastError().text());
        return false;
    }

    if (!db.isOpen()) {
        qDebug("database connection is not open. trying to open it...");

        if (m_model->status(connection) == Connection::REQUIRE_PASSWORD) {
            QString password;
            int ret = readCredentials(connection, password);

            if (ret != SQLManager::K_WALLET_CONNECTION_SUCCESSFUL) {
                qDebug("Can't retrieve password from kwallet. returned code %d", ret);
            } else {
                db.setPassword(password);
                m_model->setPassword(connection, password);
            }
        }

        if (!db.open()) {
            m_model->setStatus(connection, Connection::OFFLINE);
            Q_EMIT error(db.lastError().text());
            return false;
        }
    }

    m_model->setStatus(connection, Connection::ONLINE);

    return true;
}

void SQLManager::reopenConnection(const QString &name)
{
    Q_EMIT connectionAboutToBeClosed(name);

    QSqlDatabase db = QSqlDatabase::database(name);

    db.close();
    isValidAndOpen(name);
}

int SQLManager::storeCredentials(const Connection &conn)
{
    QJsonObject map;
    map[QStringLiteral("driver")] = conn.driver.toUpper();
    map[QStringLiteral("options")] = conn.options;

    // Sqlite is without password
    if (conn.driver.contains(QLatin1String("QSQLITE"))) {
        map[QStringLiteral("database")] = conn.database;
    } else {
        map[QStringLiteral("database")] = conn.database.toUpper();
        map[QStringLiteral("username")] = conn.username;
        map[QStringLiteral("password")] = conn.password;
        map[QStringLiteral("hostname")] = conn.hostname.toUpper();
        map[QStringLiteral("port")] = QString::number(conn.port);
    }

    // store the full map just as binary key as JSON
    QKeychain::WritePasswordJob job(QStringLiteral("org.kde.kate.katesql"));
    job.setAutoDelete(false);
    job.setKey(conn.name);
    job.setBinaryData(QJsonDocument(map).toJson(QJsonDocument::Compact));

    // we need to have a blocking API
    QEventLoop loop;
    connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    return job.error() ? SQLManager::K_WALLET_CONNECTION_ERROR : SQLManager::K_WALLET_CONNECTION_SUCCESSFUL;
}

// if success, password contain the password
int SQLManager::readCredentials(const QString &name, QString &password)
{
    // get the full map just as binary key as JSON
    QKeychain::ReadPasswordJob job(QStringLiteral("org.kde.kate.katesql"));
    job.setAutoDelete(false);
    job.setKey(name);

    // we need to have a blocking API
    QEventLoop loop;
    connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    if (!job.error()) {
        // check if data makes any sense
        const QJsonObject map = QJsonDocument::fromJson(job.binaryData()).object();
        if (!map.contains(QStringLiteral("password"))) {
            password = map.value(QStringLiteral("password")).toString();
            return SQLManager::K_WALLET_CONNECTION_SUCCESSFUL;
        }
    }
    return SQLManager::K_WALLET_CONNECTION_ERROR;
}

ConnectionModel *SQLManager::connectionModel()
{
    return m_model;
}

void SQLManager::removeConnection(const QString &name)
{
    Q_EMIT connectionAboutToBeClosed(name);

    m_model->removeConnection(name);

    QSqlDatabase::removeDatabase(name);

    Q_EMIT connectionRemoved(name);
}

void SQLManager::loadConnections(const KConfigGroup &connectionsGroup)
{
    Connection c;
    const auto groupList = connectionsGroup.groupList();

    for (const QString &groupName : groupList) {
        qDebug("reading group: %ls", qUtf16Printable(groupName));

        KConfigGroup group = connectionsGroup.group(groupName);

        c.name = groupName;
        c.driver = group.readEntry("driver");
        c.options = group.readEntry("options");

        if (c.driver.contains(QLatin1String("QSQLITE"))) {
            c.database = QUrl(group.readEntry("database")).path();
        } else {
            c.database = group.readEntry("database");
            c.hostname = group.readEntry("hostname");
            c.username = group.readEntry("username");
            c.port = group.readEntry("port", 0);

            // for compatibility with version 0.2, when passwords
            // were stored in config file instead of kwallet
            c.password = group.readEntry("password");

            if (!c.password.isEmpty()) {
                c.status = Connection::ONLINE;
            } else {
                c.status = Connection::REQUIRE_PASSWORD;
            }
        }
        createConnection(c);
    }
}

void SQLManager::saveConnections(KConfigGroup *connectionsGroup)
{
    //    qDebug() << "Saving " << m_model->rowCount() << " groups";
    for (int i = 0; i < m_model->rowCount(); i++) {
        saveConnection(connectionsGroup, m_model->data(m_model->index(i), Qt::UserRole).value<Connection>());
    }
}

void SQLManager::saveConnection(KConfigGroup *connectionsGroup, const Connection &conn)
{
    //    qDebug() << "saving connection " << conn.name;
    KConfigGroup group = connectionsGroup->group(conn.name);

    group.writeEntry("driver", conn.driver);
    group.writeEntry("options", conn.options);

    if (conn.driver.contains(QLatin1String("QSQLITE"))) {
        group.writeEntry("database", QUrl::fromLocalFile(conn.database));
        return;
    }
    group.writeEntry("database", conn.database);
    group.writeEntry("hostname", conn.hostname);
    group.writeEntry("username", conn.username);
    group.writeEntry("port", conn.port);
}

void SQLManager::runQuery(const QString &text, const QString &connection)
{
    //    qDebug() << "connection:" << connection;
    //    qDebug() << "text:" << text;

    if (text.isEmpty()) {
        return;
    }

    if (!isValidAndOpen(connection)) {
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(connection);
    QSqlQuery query(db);

    if (!query.prepare(text)) {
        QSqlError err = query.lastError();
        const int res = QMessageBox::warning(
            qApp->activeWindow(),
            i18n("Prepare Statement Failure"),
            i18n("<p>Preparing the query failed with the following error: %1</p><p>Do you want to continue without preparing the query?</p>", err.text()),
            QMessageBox::Yes,
            QMessageBox::No);

        if (res == QMessageBox::Rejected) {
            if (err.type() == QSqlError::ConnectionError) {
                m_model->setStatus(connection, Connection::OFFLINE);
            }

            Q_EMIT error(err.text());
            return;
        }
    }

    if (!query.exec()) {
        QSqlError err = query.lastError();

        if (err.type() == QSqlError::ConnectionError) {
            m_model->setStatus(connection, Connection::OFFLINE);
        }

        Q_EMIT error(err.text());
        return;
    }

    QString message;

    /// TODO: improve messages
    if (query.isSelect()) {
        if (!query.driver()->hasFeature(QSqlDriver::QuerySize)) {
            message = i18nc("@info", "Query completed successfully");
        } else {
            int nRowsSelected = query.size();
            message = i18ncp("@info", "%1 record selected", "%1 records selected", nRowsSelected);
        }
    } else {
        int nRowsAffected = query.numRowsAffected();
        message = i18ncp("@info", "%1 row affected", "%1 rows affected", nRowsAffected);
    }

    Q_EMIT success(message);
    Q_EMIT queryActivated(query, connection);
}

#include "moc_sqlmanager.cpp"
