/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "sqlbatchexecutor.h"
#include "sqlqueryscannerstatemachine.h"
#include "sqlqueryworker.h"

#include <KTextEditor/Document>

#include <KLocalizedString>
#include <QSqlDatabase>
#include <QThread>
#include <QTimer>
#include <QUuid>

SQLBatchExecutor::SQLBatchExecutor(QObject *parent)
    : QObject(parent)
{
}

SQLBatchExecutor::~SQLBatchExecutor()
{
    cleanupWorker();
}

void SQLBatchExecutor::executeBatch(KTextEditor::Document *doc,
                                    const Connection &connection,
                                    const bool blankLineBreaksStatements,
                                    const KTextEditor::Range range)
{
    if (m_running || !doc) {
        return;
    }

    m_running = true;
    m_completedCount = 0;
    m_failedCount = 0;
    m_currentIndex = 0;
    m_statementInFlight = false;
    m_cancelFlag.storeRelaxed(0);
    m_currentDoc = doc;
    m_statementRanges.clear();

    Q_EMIT batchStarted();
    m_batchTimer.start();

    // Initialize chunked scanner state (no actual scanning yet).
    m_scanState = SQLQueryScannerStateMachine::createChunkedScanState(doc, blankLineBreaksStatements, range);

    if (m_scanState.done) {
        finishBatch();
        return;
    }

    // Create worker thread for async execution.
    m_dbConnectionName = QStringLiteral("katesql_batch_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));

    m_workerThread = new QThread();
    m_worker = new SQLQueryWorker(connection, m_dbConnectionName);
    m_worker->moveToThread(m_workerThread);

    connect(
        m_worker,
        &SQLQueryWorker::statementResult,
        this,
        [this](bool success, const QString &message, qint64 elapsedMs) {
            if (!m_running) {
                return;
            }
            m_statementInFlight = false;
            if (success) {
                m_completedCount++;
            } else {
                m_failedCount++;
            }
            Q_EMIT statementResult(success, message, elapsedMs);
            dispatchNextStatement();
        },
        Qt::QueuedConnection);

    connect(
        m_worker,
        &SQLQueryWorker::connectionLost,
        this,
        [this](const QString &error) {
            if (!m_running) {
                return;
            }
            m_statementInFlight = false;
            Q_EMIT statementResult(false, error, 0);
            cancel();
        },
        Qt::QueuedConnection);

    m_workerThread->start();

    // #11: Watch for document mutations during batch execution.
    // If the user edits the document, cached ranges become stale and
    // dispatching text for them would execute the wrong statements.
    // Cancel the batch and report the interruption.
    m_docTextChangedConnection = connect(doc, &KTextEditor::Document::textChanged, this, &SQLBatchExecutor::onDocumentTextChanged, Qt::UniqueConnection);

    // Kick off the first scan chunk (scanning resumes on the event loop).
    QTimer::singleShot(0, this, &SQLBatchExecutor::scanNextChunk);
}

void SQLBatchExecutor::scanNextChunk()
{
    if (!m_running) {
        return;
    }

    if (m_cancelFlag.loadRelaxed() || !m_currentDoc) {
        tryFinishBatch();
        return;
    }

    // Scan up to kLinesPerChunk lines, appending found ranges.
    SQLQueryScannerStateMachine::scanStatementsChunk(m_currentDoc, m_scanState, kLinesPerChunk, m_statementRanges);

    // Dispatch a statement if the worker is idle and we have one buffered.
    if (!m_statementInFlight) {
        dispatchNextStatement();
    }

    if (!m_scanState.done) {
        // More lines to scan — yield to the event loop, then continue.
        QTimer::singleShot(0, this, &SQLBatchExecutor::scanNextChunk);
    } else {
        // Scanning complete.  If no statement is in flight and all ranges
        // have been dispatched, we're done.  Otherwise the
        // statementResult handler will call tryFinishBatch() when the
        // last statement completes.
        if (!m_statementInFlight && m_currentIndex >= m_statementRanges.size()) {
            finishBatch();
        }
        // else: wait for the last statement to complete, which will
        // trigger dispatchNextStatement → tryFinishBatch → finishBatch.
    }
}

void SQLBatchExecutor::dispatchNextStatement()
{
    if (m_cancelFlag.loadRelaxed()) {
        tryFinishBatch();
        return;
    }

    // Document was closed mid-batch
    if (!m_currentDoc) {
        tryFinishBatch();
        return;
    }

    // Don't dispatch if a statement is already running.
    if (m_statementInFlight) {
        return;
    }

    while (m_currentIndex < m_statementRanges.size()) {
        if (m_cancelFlag.loadRelaxed()) {
            tryFinishBatch();
            return;
        }

        if (!m_currentDoc) {
            tryFinishBatch();
            return;
        }

        QString text = m_currentDoc->text(m_statementRanges[m_currentIndex]);
        m_currentIndex++;

        // In-place right-trim. Left side is already trimmed by the scanner
        // (stmtStartCol points to first non-space, non-comment character).
        // This avoids allocating a full copy like QString::trimmed() would,
        // which matters for huge statements (e.g. multi-GB INSERT with BLOB data).
        while (!text.isEmpty() && text.back().isSpace()) {
            text.chop(1);
        }

        if (text.isEmpty()) {
            continue; // skip empty
        }

        // Dispatch to worker thread.
        m_statementInFlight = true;
        auto *worker = m_worker;
        QMetaObject::invokeMethod(
            worker,
            [worker, text = std::move(text)]() {
                worker->executeStatement(text);
            },
            Qt::QueuedConnection);
        return; // statementResult will call us back
    }

    // No more buffered statements to dispatch right now.
    // Check if we can finish (scanning done + all dispatched).
    tryFinishBatch();
}

void SQLBatchExecutor::tryFinishBatch()
{
    // Only finish when both scanning is complete AND all statements have
    // been executed (or there's nothing to execute).
    if (m_scanState.done && !m_statementInFlight && m_currentIndex >= m_statementRanges.size()) {
        finishBatch();
    }
    // Otherwise: either scanning is still in progress (scanNextChunk will
    // call tryFinishBatch when done), or a statement is in flight
    // (statementResult handler will call dispatchNextStatement → tryFinishBatch).
}

void SQLBatchExecutor::cancel()
{
    m_cancelFlag.storeRelaxed(1);
    if (m_worker) {
        m_worker->cancel();
    }
    // Mark scanning as done to prevent scanNextChunk from scheduling more work.
    m_scanState.done = true;

    // The worker emits statementResult even when cancelled (guaranteeing
    // dispatchNextStatement will be called).  As an extra safety net, if
    // no statement is in flight, schedule a deferred finish.
    QMetaObject::invokeMethod(
        this,
        [this]() {
            if (m_running) {
                tryFinishBatch();
            }
        },
        Qt::QueuedConnection);
}

void SQLBatchExecutor::finishBatch()
{
    if (!m_running) {
        return;
    }

    cleanupWorker();

    const qint64 totalMs = m_batchTimer.elapsed();
    m_running = false;
    m_statementRanges.clear();
    m_statementRanges.squeeze();
    m_currentDoc = nullptr;
    m_statementInFlight = false;

    Q_EMIT batchFinished(m_completedCount, m_failedCount, totalMs);
}

void SQLBatchExecutor::cleanupWorker()
{
    // Disconnect document mutation watcher before tearing down the worker.
    if (m_docTextChangedConnection) {
        disconnect(m_docTextChangedConnection);
        m_docTextChangedConnection = {};
    }

    if (m_worker) {
        disconnect(m_worker, nullptr, this, nullptr);
    }
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }

    if (m_worker) {
        if (m_workerThread) {
            disconnect(m_workerThread, nullptr, m_worker, nullptr);
        }
        delete m_worker;
        m_worker = nullptr;
    }

    if (m_workerThread) {
        delete m_workerThread;
        m_workerThread = nullptr;
    }

    if (!m_dbConnectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(m_dbConnectionName);
        m_dbConnectionName.clear();
    }
}

bool SQLBatchExecutor::isRunning() const
{
    return m_running;
}

void SQLBatchExecutor::onDocumentTextChanged()
{
    if (!m_running) {
        return;
    }

    // The document was modified while a batch is running — cached ranges
    // no longer correspond to the correct text.  Cancel the batch and
    // report the interruption so the user knows statements were skipped.
    const int dispatched = m_currentIndex;
    const int total = m_statementRanges.size();
    const int skipped = total - dispatched - (m_statementInFlight ? 1 : 0);

    cancel();

    Q_EMIT statementResult(false, i18nc("@info", "Document modified during execution — batch cancelled (%1 of %2 statements skipped).", skipped, total), 0);
}
