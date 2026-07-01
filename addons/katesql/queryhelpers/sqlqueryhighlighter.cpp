/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "sqlqueryhighlighter.h"
#include "katesqlconstants.h"
#include "sqlqueryscannerstatemachine.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QGuiApplication>
#include <QPalette>

#include <climits>
#include <qlatin1stringview.h>

SQLQueryHighlighter::SQLQueryHighlighter(KTextEditor::MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
{
}

SQLQueryHighlighter::~SQLQueryHighlighter()
{
    clearQueryHighlight();
}

void SQLQueryHighlighter::updateConfigCache()
{
    if (m_queryHighlightAttribute) {
        QColor highlightColor = qApp->palette().highlight().color();
        highlightColor.setAlpha(70);
        m_queryHighlightAttribute->setBackground(highlightColor);
    }

    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    m_blankLineBreaksStatements =
        config.readEntry(KateSQLConstants::Config::TreatBlankLineAsStatementBreak, KateSQLConstants::Config::DefaultValues::TreatBlankLineAsStatementBreak);
}

bool SQLQueryHighlighter::isEnabledInConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    return config.readEntry(KateSQLConstants::Config::EnableAutoQuerySelection, KateSQLConstants::Config::DefaultValues::EnableAutoQuerySelection);
}

bool SQLQueryHighlighter::isBlankLineBreakEnabled() const
{
    return m_blankLineBreaksStatements;
}

void SQLQueryHighlighter::setup()
{
    m_queryHighlightAttribute = new KTextEditor::Attribute();

    updateConfigCache();

    m_queryHighlightDebouncingTimer.setInterval(100);
    m_queryHighlightDebouncingTimer.setSingleShot(true);
    m_queryHighlightDebouncingTimer.callOnTimeout(this, &SQLQueryHighlighter::updateQueryHighlight);

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &SQLQueryHighlighter::slotViewChanged);

    // Initial highlight for current view (member timer so tearDown() can cancel it)
    m_initialHighlightTimer.setSingleShot(true);
    m_initialHighlightTimer.setInterval(50);
    m_initialHighlightTimer.callOnTimeout(this, [this]() {
        if (auto *view = m_mainWindow->activeView()) {
            slotViewChanged(view);
        }
    });
    m_initialHighlightTimer.start();
}

void SQLQueryHighlighter::tearDown()
{
    // Disconnect all signal-slot connections established in setup()
    disconnect(m_mainWindow, nullptr, this, nullptr);

    // Disconnect from active view and its document
    if (m_activeView) {
        disconnect(m_activeView, nullptr, this, nullptr);
        if (auto *doc = m_activeView->document()) {
            disconnect(doc, nullptr, this, nullptr);
        }
    }

    m_queryHighlightDebouncingTimer.stop();
    m_initialHighlightTimer.stop();
    clearQueryHighlight();
    m_activeView = nullptr;
    m_queryHighlightAttribute = nullptr;
}

bool SQLQueryHighlighter::isSqlDocument(KTextEditor::Document *doc)
{
    if (!doc) {
        return false;
    }

    const QString fileName = doc->url().fileName();
    if (fileName.endsWith(QLatin1String(".sql"), Qt::CaseInsensitive)) {
        return true;
    }
    return doc->highlightingMode().compare(QLatin1String("sql"), Qt::CaseInsensitive) == 0;
}

void SQLQueryHighlighter::slotViewChanged(KTextEditor::View *view)
{
    if (view == m_activeView) {
        return;
    }

    // Disconnect from previous view
    if (m_activeView) {
        disconnect(m_activeView, nullptr, this, nullptr);

        auto *prevDoc = m_activeView->document();
        if (prevDoc) {
            disconnect(prevDoc, nullptr, this, nullptr);
        }
    }

    m_activeView = view;
    clearQueryHighlight();

    if (!view) {
        return;
    }

    auto *doc = view->document();

    // Connect to new view's signals
    connect(view, &KTextEditor::View::cursorPositionChanged, this, &SQLQueryHighlighter::slotCursorPositionChanged, Qt::UniqueConnection);
    connect(doc, &KTextEditor::Document::textChanged, this, &SQLQueryHighlighter::slotTextChanged, Qt::UniqueConnection);
    connect(doc, &KTextEditor::Document::documentUrlChanged, this, &SQLQueryHighlighter::slotDocumentUrlChanged, Qt::UniqueConnection);
#if KTEXTEDITOR_VERSION < QT_VERSION_CHECK(6, 9, 0)
    connect(doc, &KTextEditor::Document::aboutToDeleteMovingInterfaceContent, this, &SQLQueryHighlighter::clearQueryHighlight, Qt::UniqueConnection);
#endif
    connect(doc, &KTextEditor::Document::aboutToInvalidateMovingInterfaceContent, this, &SQLQueryHighlighter::clearQueryHighlight, Qt::UniqueConnection);

    if (isSqlDocument(doc)) {
        updateQueryHighlight();
    }
}

void SQLQueryHighlighter::slotCursorPositionChanged(KTextEditor::View *view)
{
    if (!view || !isSqlDocument(view->document())) {
        return;
    }

    const auto cursor = view->cursorPosition();

    if (!m_cachedQueryRanges.isEmpty()) {
        // Case 1: cursor is still within the innermost (highlighted) range.
        // It might have entered a deeper nested subquery that wasn't previously
        // tracked, so schedule a reparse to detect that.  Don't clear the
        // highlight to avoid flicker.
        if (m_cachedQueryRanges.first().contains(cursor)) {
            if (!m_queryHighlightDebouncingTimer.isActive()) {
                m_queryHighlightDebouncingTimer.start();
            }
            return;
        }

        // Case 2: cursor left the innermost range.  Check if it landed in a
        // cached parent range — if so, promote the highlight instantly (no
        // parsing needed) and schedule a delayed reparse as a safety net for
        // edge cases (e.g. cursor landed in a sibling subquery).
        for (int i = 1; i < m_cachedQueryRanges.size(); ++i) {
            if (m_cachedQueryRanges[i].contains(cursor)) {
                // Promote: drop deeper ranges, keep this parent as innermost.
                m_cachedQueryRanges = m_cachedQueryRanges.mid(i);
                if (m_queryHighlightRange && m_queryHighlightRange->document() == view->document()) {
                    m_queryHighlightRange->setRange(m_cachedQueryRanges.first());
                }
                if (!m_queryHighlightDebouncingTimer.isActive()) {
                    m_queryHighlightDebouncingTimer.start();
                }
                return;
            }
        }
    }

    // Case 3: cursor outside all cached ranges, or cache is empty.
    clearQueryHighlight();
    if (!m_queryHighlightDebouncingTimer.isActive()) {
        m_queryHighlightDebouncingTimer.start();
    }
}

void SQLQueryHighlighter::slotTextChanged()
{
    auto view = m_activeView;
    if (view && isSqlDocument(view->document())) {
        m_cachedQueryRanges.clear();
        if (!m_queryHighlightDebouncingTimer.isActive()) {
            m_queryHighlightDebouncingTimer.start();
        }
    }
}

void SQLQueryHighlighter::slotDocumentUrlChanged(KTextEditor::Document *doc)
{
    // The mode might have changed when the document URL changes
    Q_UNUSED(doc);
    clearQueryHighlight();
    if (m_activeView && isSqlDocument(m_activeView->document())) {
        updateQueryHighlight();
    }
}

void SQLQueryHighlighter::clearQueryHighlight()
{
    m_queryHighlightRange.reset();
    m_cachedQueryRanges.clear();
}

QList<KTextEditor::Range> SQLQueryHighlighter::currentQueryRanges() const
{
    return m_cachedQueryRanges;
}

QList<KTextEditor::Range> SQLQueryHighlighter::findCurrentQueryRanges(KTextEditor::Document *doc, KTextEditor::Cursor cursor, bool blankLineBreaksStatements)
{
    if (!doc || !cursor.isValid()) {
        return {};
    }

    const int totalLines = doc->lines();
    if (totalLines == 0) {
        return {};
    }

    const int cursorLine = cursor.line();

    // Step 1: Find a conservative starting line for the forward scan.
    int searchStartLine = SQLQueryScannerStateMachine::findSearchStartLine(doc, cursorLine, cursor.column(), blankLineBreaksStatements);

    // Step 2: Forward scan — unified token scanner handles both statement
    // detection and SELECT-paren collection in a single pass.
    SQLQueryScannerStateMachine::ScanResult scan = SQLQueryScannerStateMachine::scanTokens(
        doc,
        cursor,
        searchStartLine,
        0,
        totalLines - 1,
        INT_MAX,
        true, // track statements
        cursorLine + KateSQLConstants::Config::DefaultValues::MaxLinesAroundTheCursorToScanForHighlight, // todo make max lines configurable
        blankLineBreaksStatements);

    if (!scan.foundEnclosing || !scan.enclosingStatementRange.isValid()) {
        return {};
    }

    // Step 3: Build trimmed content ranges from SELECT paren pairs.
    QList<KTextEditor::Range> result = SQLQueryScannerStateMachine::buildTrimmedRanges(doc, scan.selectParens);

    // Step 4: Check if the statement starts with EXPLAIN and add a nesting
    // level for the inner query.  Known modifiers (ANALYZE, VERBOSE, …) are
    // skipped so the sub-range starts at the actual query:
    //   "EXPLAIN SELECT ..."             → sub-range: "SELECT ..."
    //   "EXPLAIN ANALYZE SELECT ..."     → sub-range: "SELECT ..."
    //   "EXPLAIN ANALYZE VERBOSE UPDATE" → sub-range: "UPDATE ..."
    if (scan.stmtFirstKeyword == SQLQueryScannerStateMachine::FirstKeyword::Explain) {
        const int endLine = scan.enclosingStatementRange.end().line();
        KTextEditor::Cursor pos = scan.enclosingStatementRange.start();

        // Skip past EXPLAIN
        KTextEditor::Cursor afterExplain =
            SQLQueryScannerStateMachine::skipWhitespaceAndComments(doc, pos.line(), pos.column() + QLatin1String("EXPLAIN").size(), endLine);

        // Skip known EXPLAIN modifiers and parenthesized option lists.
        // Handles both syntaxes in any combination:
        //   EXPLAIN ANALYZE VERBOSE SELECT …
        //   EXPLAIN (ANALYZE, BUFFERS, VERBOSE) SELECT …
        //   EXPLAIN ANALYZE (BUFFERS, TIMING) SELECT …  (unlikely but safe)
        static const QLatin1String explainModifiers[] = {
            QLatin1String("ANALYZE"),
            QLatin1String("VERBOSE"),
            QLatin1String("BUFFERS"),
            QLatin1String("TIMING"),
            QLatin1String("COSTS"),
            QLatin1String("SETTINGS"),
            QLatin1String("SUMMARY"),
            QLatin1String("MEMORY"),
        };
        bool matched = true;
        while (matched && afterExplain.isValid()) {
            matched = false;
            for (const auto &mod : explainModifiers) {
                KTextEditor::Cursor afterMod = SQLQueryScannerStateMachine::trySkipKeyword(doc, afterExplain.line(), afterExplain.column(), mod, endLine);
                if (afterMod.isValid()) {
                    afterExplain = afterMod;
                    matched = true;
                    break; // restart from new position
                }
            }
            if (!matched) {
                KTextEditor::Cursor afterParens =
                    SQLQueryScannerStateMachine::trySkipParenthesizedOptions(doc, afterExplain.line(), afterExplain.column(), endLine);
                if (afterParens.isValid()) {
                    afterExplain = afterParens;
                    matched = true;
                }
            }
        }

        if (afterExplain.isValid() && afterExplain < scan.enclosingStatementRange.end()) {
            KTextEditor::Range explainRange(afterExplain, scan.enclosingStatementRange.end());
            if (cursor >= afterExplain && cursor <= scan.enclosingStatementRange.end()) {
                result.append(explainRange);
            }
        }
    }

    // Append the full statement range as the last element (outermost)
    result.append(scan.enclosingStatementRange);

    return result;
}

bool SQLQueryHighlighter::tryIncrementalCacheUpdate(KTextEditor::Document *doc, KTextEditor::Cursor cursor)
{
    if (!doc || !cursor.isValid() || m_cachedQueryRanges.isEmpty()) {
        return false;
    }

    const KTextEditor::Range &innermost = m_cachedQueryRanges.first();
    if (!innermost.contains(cursor)) {
        return false;
    }

    // Bounded forward scan within innermost range for deeper (SELECT ...) pairs
    // that enclose the cursor.  No statement tracking needed.
    //
    // Boundary note: scanTokens uses col < colEnd, and innermost.end().column()
    // is exclusive (one past last char), so the scan covers all characters in
    // the range [start, end) exactly — characters aren't missed at the boundary.
    SQLQueryScannerStateMachine::ScanResult scan = SQLQueryScannerStateMachine::scanTokens(doc,
                                                                                           cursor,
                                                                                           innermost.start().line(),
                                                                                           innermost.start().column(),
                                                                                           innermost.end().line(),
                                                                                           innermost.end().column(),
                                                                                           false, // no statement tracking
                                                                                           -1); // no early termination

    if (scan.selectParens.isEmpty()) {
        // No deeper nesting found — cache is still valid, no changes needed
        return true;
    }

    // Trim whitespace inside found paren pairs and build new ranges.
    QList<KTextEditor::Range> newRanges = SQLQueryScannerStateMachine::buildTrimmedRanges(doc, scan.selectParens);

    if (newRanges.isEmpty()) {
        return true;
    }

    // Prepend new deeper ranges (already innermost-first) before existing cache
    m_cachedQueryRanges = newRanges + m_cachedQueryRanges;

    // Update highlight to new innermost range
    applyHighlightRange(doc, m_cachedQueryRanges.first());

    return true;
}

void SQLQueryHighlighter::updateQueryHighlight()
{
    auto view = m_activeView;
    if (!view) {
        clearQueryHighlight();
        return;
    }

    auto *doc = view->document();
    if (!doc || !isSqlDocument(doc)) {
        clearQueryHighlight();
        return;
    }

    // If there's a selection, don't highlight (the selection itself is the query)
    if (view->selection()) {
        clearQueryHighlight();
        return;
    }

    // Fast path: if the cursor is still within the innermost cached range,
    // try an incremental update (scan only within that range for deeper nesting).
    // This avoids a full rescan from the nearest semicolon.
    if (tryIncrementalCacheUpdate(doc, view->cursorPosition())) {
        return;
    }

    QList<KTextEditor::Range> queryRanges = findCurrentQueryRanges(doc, view->cursorPosition(), m_blankLineBreaksStatements);

    if (queryRanges.isEmpty()) {
        clearQueryHighlight();
        return;
    }

    // Highlight the narrowest (first) range by default — the innermost subquery
    KTextEditor::Range highlightRange = queryRanges.first();

    // If we already have a range and it matches, just update cache
    if (m_queryHighlightRange && m_queryHighlightRange->toRange() == highlightRange && m_cachedQueryRanges == queryRanges) {
        return;
    }

    // Update cache
    m_cachedQueryRanges = queryRanges;

    // Apply the highlight range (reuses existing MovingRange if possible)
    applyHighlightRange(doc, highlightRange);
}

void SQLQueryHighlighter::applyHighlightRange(KTextEditor::Document *doc, KTextEditor::Range range)
{
    if (m_queryHighlightRange && m_queryHighlightRange->document() == doc) {
        m_queryHighlightRange->setRange(range);
    } else {
        m_queryHighlightRange.reset();
        m_queryHighlightRange.reset(doc->newMovingRange(range));
        m_queryHighlightRange->setAttribute(m_queryHighlightAttribute);
        m_queryHighlightRange->setView(m_activeView);
    }
}

void SQLQueryHighlighter::setPreviewRange(const KTextEditor::Range &range)
{
    // Stop debouncing timer while popup preview is active so it doesn't
    // overwrite the preview with the default highlight range.
    m_queryHighlightDebouncingTimer.stop();

    if (!m_activeView) {
        return;
    }

    auto *doc = m_activeView->document();
    if (!doc) {
        return;
    }

    KTextEditor::Range highlightRange = range;

    // If invalid, restore the default (innermost cached range)
    if (!highlightRange.isValid() && !m_cachedQueryRanges.isEmpty()) {
        highlightRange = m_cachedQueryRanges.first();
    }

    if (!highlightRange.isValid()) {
        m_queryHighlightRange.reset();
        return;
    }

    applyHighlightRange(doc, highlightRange);
}

void SQLQueryHighlighter::clearPreview()
{
    m_queryHighlightDebouncingTimer.stop();
    m_queryHighlightRange.reset();
}

#include "moc_sqlqueryhighlighter.cpp"
