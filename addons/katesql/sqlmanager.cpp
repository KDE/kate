/*
 *   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-only
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
#include <QSqlQuery>
#include <QStringList>

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
        qDebug("connection %ls already exists", qUtf16Printable(conn.name));
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

    if (db.open()) {
        m_model->setStatus(conn.name, Connection::ONLINE);
    } else {
        m_model->setStatus(conn.name, Connection::OFFLINE);
        Q_EMIT error(db.lastError().text());
    }

    Q_EMIT connectionCreated(conn.name);
}

bool SQLManager::testConnection(const Connection &conn, QSqlError &error)
{
    QString connectionName = conn.name.isEmpty() ? QStringLiteral("katesql-test") : conn.name;
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
        if (m_model->status(connection) == Connection::REQUIRE_PASSWORD) {
            QString password;
            int ret = readCredentials(connection, password);
            if (ret == SQLManager::K_WALLET_CONNECTION_SUCCESSFUL) {
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

    if (conn.driver.contains(QLatin1String("QSQLITE"))) {
        map[QStringLiteral("database")] = conn.database;
    } else {
        map[QStringLiteral("database")] = conn.database.toUpper();
        map[QStringLiteral("username")] = conn.username;
        map[QStringLiteral("password")] = conn.password;
        map[QStringLiteral("hostname")] = conn.hostname.toUpper();
        map[QStringLiteral("port")] = QString::number(conn.port);
    }

    QKeychain::WritePasswordJob job(QStringLiteral("org.kde.kate.katesql"));
    job.setAutoDelete(false);
    job.setKey(conn.name);
    job.setBinaryData(QJsonDocument(map).toJson(QJsonDocument::Compact));

    QEventLoop loop;
    connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    return job.error() ? SQLManager::K_WALLET_CONNECTION_ERROR : SQLManager::K_WALLET_CONNECTION_SUCCESSFUL;
}

int SQLManager::readCredentials(const QString &name, QString &password)
{
    QKeychain::ReadPasswordJob job(QStringLiteral("org.kde.kate.katesql"));
    job.setAutoDelete(false);
    job.setKey(name);

    QEventLoop loop;
    connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();

    if (!job.error()) {
        const QJsonObject map = QJsonDocument::fromJson(job.binaryData()).object();
        if (map.contains(QStringLiteral("password"))) {
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
            c.password = group.readEntry("password");
            c.status = c.password.isEmpty() ? Connection::REQUIRE_PASSWORD : Connection::ONLINE;
        }
        createConnection(c);
    }
}

void SQLManager::saveConnections(KConfigGroup *connectionsGroup)
{
    for (int i = 0; i < m_model->rowCount(); i++) {
        saveConnection(connectionsGroup, m_model->data(m_model->index(i), Qt::UserRole).value<Connection>());
    }
}

void SQLManager::saveConnection(KConfigGroup *connectionsGroup, const Connection &conn)
{
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
    if (text.isEmpty() || !isValidAndOpen(connection)) {
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(connection);
    QSqlQuery query(db);

    // Split multiple queries properly, handling semicolons within strings and complex statements
    QStringList statements;
    QString currentStatement;
    bool inString = false;
    QChar stringChar;
    bool inComment = false;

    for (int i = 0; i < text.length(); ++i) {
        QChar c = text[i];
        QChar nextChar = (i + 1 < text.length()) ? text[i + 1] : QChar();

        // Handle comments
        if (!inString && !inComment && c == QLatin1Char('-') && nextChar == QLatin1Char('-')) {
            // Skip to end of line
            while (i < text.length() && text[i] != QLatin1Char('\n')) {
                i++;
            }
            continue;
        }

        if (!inString && !inComment && c == QLatin1Char('/') && nextChar == QLatin1Char('*')) {
            inComment = true;
            i++; // Skip the next character
            continue;
        }

        if (inComment && c == QLatin1Char('*') && nextChar == QLatin1Char('/')) {
            inComment = false;
            i++; // Skip the next character
            continue;
        }

        if (inComment) {
            continue;
        }

        // Handle strings
        if (!inComment && (c == QLatin1Char('\'') || c == QLatin1Char('"'))) {
            if (!inString) {
                inString = true;
                stringChar = c;
            } else if (c == stringChar) {
                // Check for escaped quotes
                if (i > 0 && text[i-1] == QLatin1Char('\\')) {
                    // This is an escaped quote, continue
                } else {
                    inString = false;
                }
            }
        }

        // Handle statement separation
        if (!inString && !inComment && c == QLatin1Char(';')) {
            QString trimmed = currentStatement.trimmed();
            if (!trimmed.isEmpty()) {
                statements.append(trimmed);
            }
            currentStatement.clear();
        } else {
            currentStatement.append(c);
        }
    }

    // Add the last statement if any
    QString trimmed = currentStatement.trimmed();
    if (!trimmed.isEmpty()) {
        statements.append(trimmed);
    }

    bool hasResults = false;
    int totalRowsAffected = 0;

    for (const QString &stmt : statements) {
        if (stmt.isEmpty()) continue;

        QString upper = stmt.toUpper().trimmed();

        // For PostgreSQL and other databases, we need to handle DDL statements differently
        // DDL statements (CREATE, DROP, ALTER, etc.) cannot be prepared in PostgreSQL
        bool isDDL = upper.startsWith(QLatin1String("CREATE")) ||
        upper.startsWith(QLatin1String("DROP")) ||
        upper.startsWith(QLatin1String("ALTER")) ||
        upper.startsWith(QLatin1String("TRUNCATE")) ||
        upper.startsWith(QLatin1String("COMMENT ON")) ||
        upper.startsWith(QLatin1String("GRANT")) ||
        upper.startsWith(QLatin1String("REVOKE")) ||
        upper.startsWith(QLatin1String("BEGIN")) ||
        upper.startsWith(QLatin1String("COMMIT")) ||
        upper.startsWith(QLatin1String("ROLLBACK"));

        // Check if this is a PostgreSQL-specific DDL that can't be prepared
        bool needsDirectExecution = isDDL;

        // For PostgreSQL driver, always execute DDL directly
        if (db.driverName().contains(QLatin1String("PSQL"), Qt::CaseInsensitive)) {
            needsDirectExecution = isDDL ||
            upper.startsWith(QLatin1String("VACUUM")) ||
            upper.startsWith(QLatin1String("REINDEX"));
        }

        if (needsDirectExecution) {
            // Execute DDL and other non-preparable statements directly
            if (!query.exec(stmt)) {
                QSqlError err = query.lastError();
                Q_EMIT error(i18n("Error executing statement: %1\n", err.text()));
                return;
            }
        } else {
            // For other statements, try to prepare first, then fallback to direct execution
            if (query.prepare(stmt)) {
                if (!query.exec()) {
                    QSqlError err = query.lastError();
                    Q_EMIT error(i18n("Error executing prepared statement: %1\nStatement: %2", err.text(), stmt));
                    return;
                }
            } else {
                // Preparation failed, try direct execution
                if (!query.exec(stmt)) {
                    QSqlError err = query.lastError();
                    Q_EMIT error(i18n("Error executing statement: %1\nStatement: %2", err.text(), stmt));
                    return;
                }
            }
        }

        if (query.isSelect()) {
            hasResults = true;
            // Emit the query result
            Q_EMIT queryActivated(query, connection);
            // Create a new query object for the next statement
            query = QSqlQuery(db);
        } else {
            int rowsAffected = query.numRowsAffected();
            if (rowsAffected > 0) {
                totalRowsAffected += rowsAffected;
            }
        }
    }

    // Emit appropriate success message
    if (hasResults) {
        Q_EMIT success(i18n("Query executed successfully"));
    } else if (totalRowsAffected > 0) {
        QString message = i18ncp("@info", "%1 row affected", "%1 rows affected", totalRowsAffected);
        Q_EMIT success(message);
    } else {
        Q_EMIT success(i18n("Query executed successfully"));
    }
}

#include "moc_sqlmanager.cpp"
