/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "connection.h"
#include "sqlqueryscannerstatemachine.h"

#include <KTextEditor/Range>

#include <QAtomicInt>
#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QVector>
#include <atomic>

namespace KTextEditor
{
class Document;
}

class QThread;
class SQLQueryWorker;

/// Executes SQL statements one at a time on a background thread.
/// Fully async: no nested QEventLoop — the main thread stays responsive.
///
/// Scanning and execution are pipelined: the document is scanned in small
/// chunks (yielding to the event loop between chunks), and statements are
/// dispatched to the worker thread as soon as they are found.  This means
/// execution starts before scanning finishes, and the UI remains responsive
/// (cancel works, no freeze on multi-GB files).
///
/// Flow:
///   1. executeBatch() initializes chunked scan state, starts a worker
///      thread, and schedules the first scan chunk.
///   2. scanNextChunk() scans up to N lines, appends found statement ranges,
///      and dispatches the first statement if the worker is idle.
///   3. The worker emits statementResult → the executor dispatches the next
///      buffered statement.
///   4. Scanning continues via QTimer::singleShot(0) until done.
///   5. When scanning is done AND all statements are executed, finishBatch()
///      tears down the worker thread and emits batchFinished().
class SQLBatchExecutor : public QObject
{
    Q_OBJECT
public:
    explicit SQLBatchExecutor(QObject *parent = nullptr);
    ~SQLBatchExecutor() override;

    /// Execute all statements in the given document range using a background worker.
    /// If range is invalid, scans the entire document.
    /// The connection struct must have valid credentials (caller should validate first).
    /// Does nothing if already running.
    void executeBatch(KTextEditor::Document *doc, const Connection &connection, const bool blankLineBreaksStatements, const KTextEditor::Range range);

    /// Cancel the current batch. Stops scanning and dispatching new statements.
    /// The currently-running statement (if any) completes normally.
    void cancel();

    bool isRunning() const;

Q_SIGNALS:
    /// Emitted for each statement result. Route to TextOutputWidget for display.
    void statementResult(bool success, const QString &message, qint64 elapsedMs);

    /// Emitted when a batch starts executing.
    void batchStarted();

    /// Emitted when the entire batch finishes (or is cancelled).
    void batchFinished(int completed, int failed, qint64 totalMs);

private:
    void scanNextChunk();
    void dispatchNextStatement();
    void tryFinishBatch();
    void finishBatch();
    void cleanupWorker();
    void onDocumentTextChanged();

    QThread *m_workerThread = nullptr;
    SQLQueryWorker *m_worker = nullptr;
    QAtomicInt m_cancelFlag;

    QPointer<KTextEditor::Document> m_currentDoc;
    QVector<KTextEditor::Range> m_statementRanges;
    int m_currentIndex = 0;

    /// True when a statement has been dispatched to the worker and we're
    /// waiting for the result.  Used to avoid dispatching two statements
    /// simultaneously.
    bool m_statementInFlight = false;

    /// Chunked scanner state — preserves position across scanNextChunk() calls.
    SQLQueryScannerStateMachine::ChunkedScanState m_scanState;

    std::atomic<bool> m_running{false};
    int m_completedCount = 0;
    int m_failedCount = 0;
    QString m_dbConnectionName;
    QElapsedTimer m_batchTimer;
    QMetaObject::Connection m_docTextChangedConnection;

    static constexpr int kLinesPerChunk = 1000;
};
