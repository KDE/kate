/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "sqlqueryworker.h"
#include "katesqlconstants.h"

#include <KLocalizedString>

#include <QElapsedTimer>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

SQLQueryWorker::SQLQueryWorker(const Connection &connection, const QString &dbConnectionName, QObject *parent)
    : QObject(parent)
    , m_connection(connection)
    , m_dbConnectionName(dbConnectionName)
{
}

SQLQueryWorker::~SQLQueryWorker()
{
    if (m_db.isValid()) {
        m_db.close();
    }
    // QSqlDatabase::removeDatabase() is called by SQLBatchExecutor on the
    // main thread after this worker is destroyed, to avoid the
    // "connection is still in use" warning.
}

void SQLQueryWorker::cancel()
{
    m_cancelled.storeRelaxed(1);
}

bool SQLQueryWorker::ensureConnectionOpen()
{
    if (m_connectionInitialized) {
        return m_db.isOpen();
    }

    m_db = QSqlDatabase::addDatabase(m_connection.driver, m_dbConnectionName);
    m_db.setHostName(m_connection.hostname);
    m_db.setDatabaseName(m_connection.database);
    m_db.setUserName(m_connection.username);
    m_db.setPassword(m_connection.password);
    m_db.setPort(m_connection.port);
    m_db.setConnectOptions(m_connection.options);

    m_connectionInitialized = true;

    if (!m_db.open()) {
        return false;
    }

    return true;
}

void SQLQueryWorker::executeStatement(const QString &text)
{
    QElapsedTimer timer;
    timer.start();

    if (m_cancelled.loadRelaxed()) {
        Q_EMIT statementResult(false, i18nc("@info", "Cancelled"), timer.elapsed());
        return;
    }

    if (!ensureConnectionOpen()) {
        const QSqlError err = m_db.lastError();
        // Emit connectionLost so the batch executor stops dispatching remaining
        // statements instead of failing each one individually.
        if (err.type() == QSqlError::ConnectionError) {
            Q_EMIT connectionLost(err.text());
        }
        Q_EMIT statementResult(false, err.text(), timer.elapsed());
        return;
    }

    QSqlQuery query(m_db);

    // Deliberately ignore prepare failure — fall through to exec().
    // Batch mode mirrors the SQLManager::Batch path where prepare
    // failures are silently skipped.
    query.prepare(text);

    if (!query.exec()) {
        const QSqlError err = query.lastError();
        const qint64 elapsedMs = timer.elapsed();

        if (err.type() == QSqlError::ConnectionError) {
            Q_EMIT connectionLost(err.text());
        }

        QString errorMsg = err.text()
            + QStringLiteral("\nQuery: %1 \nExecution took %2ms")
                  .arg(text.left(KateSQLConstants::Config::DefaultValues::MaxQueryPreviewLength), QString::number(elapsedMs));
        Q_EMIT statementResult(false, errorMsg, elapsedMs);
        return;
    }

    const qint64 elapsedMs = timer.elapsed();

    QString message;

    if (query.isSelect()) {
        if (!query.driver()->hasFeature(QSqlDriver::QuerySize)) {
            message = i18nc("@info", "Query completed successfully");
        } else {
            const int nRowsSelected = query.size();
            message = i18ncp("@info", "%1 record selected", "%1 records selected", nRowsSelected);
        }
    } else {
        const int nRowsAffected = query.numRowsAffected();
        message = i18ncp("@info", "%1 row affected", "%1 rows affected", nRowsAffected);
    }

    message += QStringLiteral(" Execution took %1ms").arg(elapsedMs);

    Q_EMIT statementResult(true, message, elapsedMs);
}
