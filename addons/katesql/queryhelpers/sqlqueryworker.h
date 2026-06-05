/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "connection.h"

#include <QAtomicInt>
#include <QObject>
#include <QSqlDatabase>

class SQLQueryWorker : public QObject
{
    Q_OBJECT
public:
    explicit SQLQueryWorker(const Connection &connection, const QString &dbConnectionName, QObject *parent = nullptr);
    ~SQLQueryWorker() override;

    /// Thread-safe. Checked by executeStatement before and after execution.
    void cancel();

public Q_SLOTS:
    /// Execute a single SQL statement. Emits statementResult when done.
    /// On the first call, lazily creates the QSqlDatabase connection.
    void executeStatement(const QString &text);

Q_SIGNALS:
    /// Emitted after each statement completes (success or failure).
    /// Also emitted when cancelled, to guarantee the caller always receives
    /// a response and can advance its state machine.
    void statementResult(bool success, const QString &message, qint64 elapsedMs);

    /// Emitted when a connection error is detected. The batch executor should
    /// stop dispatching new statements since they will all fail.
    void connectionLost(const QString &error);

private:
    bool ensureConnectionOpen();

    Connection m_connection;
    QString m_dbConnectionName;
    QSqlDatabase m_db;
    QAtomicInt m_cancelled;
    bool m_connectionInitialized = false;
};
