/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "sqlqueryscannerstatemachine.h"
#include "katesqlconstants.h"

#include <KTextEditor/Document>

inline bool SQLQueryScannerStateMachine::isBlankLine(QStringView text)
{
    return text.trimmed().isEmpty();
}

inline int SQLQueryScannerStateMachine::lastNonSpaceCol(QStringView text)
{
    for (int col = text.length() - 1; col >= 0; --col) {
        if (!text[col].isSpace())
            return col + 1;
    }
    return 0;
}

SQLQueryScannerStateMachine::CurrentCharacterAction SQLQueryScannerStateMachine::processTokenChar(QChar c, QChar next, QueryState &state)
{
    if (state.inBlockComment) {
        if (c == QLatin1Char('*') && next == QLatin1Char('/')) {
            state.inBlockComment = false;
            return CurrentCharacterAction::SkipNext;
        }
        return CurrentCharacterAction::Continue;
    }

    if (state.inString) {
        // Backslash escape: skip the next character (MySQL, PostgreSQL, etc.)
        if (c == QLatin1Char('\\')) {
            return CurrentCharacterAction::SkipNext;
        }
        if (c == state.stringChar) {
            // Doubled-quote escape: '' or ""
            if (next == state.stringChar) {
                return CurrentCharacterAction::SkipNext;
            }
            state.inString = false;
        }
        return CurrentCharacterAction::Continue;
    }

    if (state.inLineComment) {
        return CurrentCharacterAction::BreakLine;
    }

    if (c == QLatin1Char('/') && next == QLatin1Char('*')) {
        state.inBlockComment = true;
        return CurrentCharacterAction::SkipNext;
    }

    if (c == QLatin1Char('-') && next == QLatin1Char('-')) {
        state.inLineComment = true;
        return CurrentCharacterAction::SkipNext;
    }

    if (c == QLatin1Char('\'') || c == QLatin1Char('"')) {
        state.inString = true;
        state.stringChar = c;
        return CurrentCharacterAction::Continue;
    }

    if (c == QLatin1Char('(')) {
        ++state.parenDepth;
        return CurrentCharacterAction::ParenOpen;
    }

    if (c == QLatin1Char(')')) {
        if (state.parenDepth > 0) {
            --state.parenDepth;
        }
        return CurrentCharacterAction::ParenClose;
    }

    if (c == QLatin1Char(';') && state.parenDepth == 0) {
        return CurrentCharacterAction::StatementEnd;
    }

    return CurrentCharacterAction::Continue;
}

KTextEditor::Range
SQLQueryScannerStateMachine::trimmedContentRange(KTextEditor::Document *doc, const KTextEditor::Cursor &open, const KTextEditor::Cursor &close)
{
    int sLine = open.line();
    int sCol = open.column() + 1;

    bool foundStart = false;
    for (int line = sLine; line <= close.line(); ++line) {
        const QString lt = doc->line(line);
        const QChar *ld = lt.constData();
        int fromCol = (line == sLine) ? sCol : 0;
        for (int col = fromCol; col < lt.length(); ++col) {
            if (!ld[col].isSpace()) {
                sLine = line;
                sCol = col;
                foundStart = true;
                break;
            }
        }
        if (foundStart)
            break;
    }
    if (!foundStart) {
        return KTextEditor::Range::invalid();
    }

    int eLine = close.line();
    int eCol = close.column() - 1;

    bool foundEnd = false;
    for (int line = eLine; line >= sLine; --line) {
        const QString lt = doc->line(line);
        const QChar *ld = lt.constData();
        int fromCol = (line == eLine) ? eCol : lt.length() - 1;
        for (int col = fromCol; col >= 0; --col) {
            if (!ld[col].isSpace()) {
                eLine = line;
                eCol = col;
                foundEnd = true;
                break;
            }
        }
        if (foundEnd)
            break;
    }
    if (!foundEnd) {
        return KTextEditor::Range::invalid();
    }

    return KTextEditor::Range(KTextEditor::Cursor(sLine, sCol), KTextEditor::Cursor(eLine, eCol + 1));
}

KTextEditor::Cursor SQLQueryScannerStateMachine::skipWhitespaceAndComments(KTextEditor::Document *doc, int sLine, int sCol, int maxLine)
{
    bool localInBlockComment = false;
    for (int li = sLine; li <= maxLine; ++li) {
        const QString lt = doc->line(li);
        const QChar *d = lt.constData();
        const int len = lt.length();
        int fromCol = (li == sLine) ? sCol : 0;
        for (int ci = fromCol; ci < len; ++ci) {
            QChar ch = d[ci];
            QChar nxt = (ci + 1 < len) ? d[ci + 1] : QChar();

            if (localInBlockComment) {
                if (ch == QLatin1Char('*') && nxt == QLatin1Char('/')) {
                    localInBlockComment = false;
                    ++ci;
                }
                continue;
            }

            if (ch == QLatin1Char('/') && nxt == QLatin1Char('*')) {
                localInBlockComment = true;
                ++ci;
                continue;
            }

            if (ch == QLatin1Char('-') && nxt == QLatin1Char('-')) {
                break; // rest of line is comment
            }

            if (!ch.isSpace()) {
                return KTextEditor::Cursor(li, ci);
            }
        }
    }
    return KTextEditor::Cursor::invalid();
}

KTextEditor::Cursor SQLQueryScannerStateMachine::trySkipKeyword(KTextEditor::Document *doc, int line, int col, QLatin1String keyword, int maxLine)
{
    const QString lt = doc->line(line);
    if (col + keyword.size() > lt.length()) {
        return KTextEditor::Cursor::invalid();
    }
    for (int i = 0; i < keyword.size(); ++i) {
        if (lt[col + i].toUpper() != keyword[i]) {
            return KTextEditor::Cursor::invalid();
        }
    }
    // Word boundary: next char must not be a letter or underscore
    const int after = col + keyword.size();
    if (after < lt.length() && (lt[after].isLetter() || lt[after] == QLatin1Char('_'))) {
        return KTextEditor::Cursor::invalid();
    }
    return skipWhitespaceAndComments(doc, line, after, maxLine);
}

KTextEditor::Cursor SQLQueryScannerStateMachine::trySkipParenthesizedOptions(KTextEditor::Document *doc, int line, int col, int maxLine)
{
    const QString lt = doc->line(line);
    if (col >= lt.length() || lt[col] != QLatin1Char('(')) {
        return KTextEditor::Cursor::invalid();
    }

    int depth = 0;
    for (int li = line; li <= maxLine; ++li) {
        const QString lineText = doc->line(li);
        const int len = lineText.length();
        const int fromCol = (li == line) ? col : 0;
        for (int ci = fromCol; ci < len; ++ci) {
            const QChar ch = lineText[ci];
            if (ch == QLatin1Char('(')) {
                ++depth;
            } else if (ch == QLatin1Char(')')) {
                --depth;
                if (depth == 0) {
                    return skipWhitespaceAndComments(doc, li, ci + 1, maxLine);
                }
            }
        }
    }
    return KTextEditor::Cursor::invalid();
}

bool SQLQueryScannerStateMachine::containsUnquotedSemicolon(QStringView text)
{
    QueryState state;
    for (int i = 0; i < text.length(); ++i) {
        QChar c = text[i];
        QChar next = (i + 1 < text.length()) ? text[i + 1] : QChar();

        CurrentCharacterAction action = processTokenChar(c, next, state);

        if (action == CurrentCharacterAction::StatementEnd) {
            return true;
        }
        if (action == CurrentCharacterAction::BreakLine) {
            return false;
        }
        if (action == CurrentCharacterAction::SkipNext) {
            ++i;
        }
    }
    return false;
}

int SQLQueryScannerStateMachine::findSearchStartLine(KTextEditor::Document *doc, int cursorLine, int cursorColumn, bool blankLineBreaksStatements)
{
    // Check the portion of the current line before the cursor.
    const QString lineText = doc->line(cursorLine);
    if (cursorColumn > 0 && containsUnquotedSemicolon(QStringView(lineText).left(cursorColumn))) {
        return cursorLine;
    }

    const int limit =
        qMax(0, cursorLine - KateSQLConstants::Config::DefaultValues::MaxLinesAroundTheCursorToScanForHighlight); // todo make max lines configurable
    for (int line = cursorLine - 1; line >= limit; --line) {
        const QString lt = doc->line(line);
        if (blankLineBreaksStatements && isBlankLine(lt)) {
            return line;
        }
        if (containsUnquotedSemicolon(lt)) {
            return line;
        }
    }
    return 0;
}

QList<KTextEditor::Range> SQLQueryScannerStateMachine::buildTrimmedRanges(KTextEditor::Document *doc, const QVector<ParenPair> &selectParens)
{
    QList<KTextEditor::Range> ranges;
    for (const auto &pair : std::as_const(selectParens)) {
        KTextEditor::Range trimmed = trimmedContentRange(doc, pair.open, pair.close);
        if (trimmed.isValid()) {
            ranges.append(trimmed);
        }
    }
    return ranges;
}

SQLQueryScannerStateMachine::ScanResult SQLQueryScannerStateMachine::scanTokens(KTextEditor::Document *doc,
                                                                                KTextEditor::Cursor cursor,
                                                                                int startLine,
                                                                                int startCol,
                                                                                int endLine,
                                                                                int endCol,
                                                                                bool trackStatements,
                                                                                int earlyTerminateLine,
                                                                                bool blankLineBreaksStatements)
{
    ScanResult result;

    const int cursorLine = cursor.line();
    const int cursorColumn = cursor.column();

    auto cursorInStmt = [cursorLine, cursorColumn](const int sLine, const int sCol, const int eLine, const int eCol) -> bool {
        if (cursorLine < sLine || cursorLine > eLine)
            return false;
        if (cursorLine == sLine && cursorColumn < sCol)
            return false;
        if (cursorLine == eLine && cursorColumn > eCol)
            return false;
        return true;
    };

    QueryState tstate;
    QVector<ParenthesisInfo> parenStack;
    StmtTracker stmt;

    // Helper: finalize current statement if cursor is inside, else reset.
    auto tryFinalizeStatement = [&result, &stmt, cursorInStmt](const int eLine, const int eCol) -> bool {
        if (stmt.startLine >= 0 && cursorInStmt(stmt.startLine, stmt.startCol, eLine, eCol)) {
            result.enclosingStatementRange = KTextEditor::Range(KTextEditor::Cursor(stmt.startLine, stmt.startCol), KTextEditor::Cursor(eLine, eCol));
            result.stmtFirstKeyword = stmt.keyword.toFirstKeyword();
            result.foundEnclosing = true;
            return true;
        }
        stmt.reset();
        return false;
    };

    for (int line = startLine; line <= endLine && !result.foundEnclosing; ++line) {
        const QString lineText = doc->line(line);
        const QChar *d = lineText.constData();
        const int len = lineText.length();

        tstate.inLineComment = false;

        const int colStart = (line == startLine) ? startCol : 0;
        const int colEnd = (line == endLine) ? qMin(endCol, len) : len;

        // Blank line (double newline) acts as statement boundary, like ';'
        if (blankLineBreaksStatements && trackStatements && tstate.parenDepth == 0 && !tstate.inBlockComment && !tstate.inString && stmt.startLine >= 0
            && line > startLine && isBlankLine(lineText)) {
            if (tryFinalizeStatement(line - 1, lastNonSpaceCol(doc->line(line - 1)))) {
                break;
            }
            continue;
        }

        for (int col = colStart; col < colEnd; ++col) {
            QChar c = d[col];
            QChar next = (col + 1 < len) ? d[col + 1] : QChar();

            CurrentCharacterAction action = processTokenChar(c, next, tstate);

            if (action == CurrentCharacterAction::SkipNext) {
                ++col;
                continue;
            }

            if (action == CurrentCharacterAction::BreakLine) {
                break;
            }

            if (action == CurrentCharacterAction::ParenOpen) {
                if (!parenStack.isEmpty() && !parenStack.last().keyword.isDone()) {
                    parenStack.last().keyword.forceFinish();
                }
                parenStack.push_back({line, col, {}});
                continue;
            }

            if (action == CurrentCharacterAction::ParenClose) {
                if (!parenStack.isEmpty()) {
                    ParenthesisInfo openInfo = parenStack.last();
                    parenStack.pop_back();

                    KTextEditor::Cursor openCursor(openInfo.line, openInfo.col);
                    KTextEditor::Cursor closeCursor(line, col);
                    if (cursor > openCursor && cursor <= closeCursor && openInfo.keyword.isSelect()) {
                        result.selectParens.append({openCursor, closeCursor});
                    }
                }
                continue;
            }

            if (action == CurrentCharacterAction::StatementEnd) {
                if (tryFinalizeStatement(line, col + 1)) {
                    break;
                }
                continue;
            }

            // CurrentTokenState::Continue — handle non-space tracking.
            // Skip keyword feeding inside comments and strings to avoid
            // poisoning the matcher with non-SQL content.
            if (!c.isSpace() && !tstate.inBlockComment && !tstate.inLineComment && !tstate.inString) {
                const bool isLetter = c.isLetter() || c == QLatin1Char('_');

                if (trackStatements) {
                    if (stmt.startLine < 0) {
                        stmt.startLine = line;
                        stmt.startCol = col;
                        stmt.keyword.reset();
                    }
                    stmt.keyword.feed(c, isLetter);
                }

                if (!parenStack.isEmpty()) {
                    parenStack.last().keyword.feed(c, isLetter);
                }
            } else if (!tstate.inBlockComment && !tstate.inLineComment && !tstate.inString) {
                // Space terminates active keyword tracking so that e.g.
                // "EXPLAIN SELECT" yields keyword "EXPLAIN", not "EXPLAINSELECT".
                if (trackStatements) {
                    stmt.keyword.spaceTerminated();
                }
                if (!parenStack.isEmpty()) {
                    parenStack.last().keyword.spaceTerminated();
                }
            }
        }

        if (earlyTerminateLine >= 0 && line > earlyTerminateLine && stmt.startLine < 0) {
            break;
        }
    }

    // Handle last statement without trailing semicolon.
    // Use the scan bounds (endLine/endCol) rather than the full document
    // length so the result respects the requested range.
    if (trackStatements && !result.foundEnclosing && stmt.startLine >= 0 && cursorLine >= stmt.startLine) {
        for (int line = endLine; line >= stmt.startLine; --line) {
            int eCol = lastNonSpaceCol(doc->line(line));
            if (line == endLine) {
                eCol = qMin(eCol, endCol);
            }
            if (eCol > 0) {
                result.enclosingStatementRange = KTextEditor::Range(KTextEditor::Cursor(stmt.startLine, stmt.startCol), KTextEditor::Cursor(line, eCol));
                result.stmtFirstKeyword = stmt.keyword.toFirstKeyword();
                result.foundEnclosing = true;
                break;
            }
        }
    }

    return result;
}

QList<KTextEditor::Range> SQLQueryScannerStateMachine::scanAllStatements(const QStringList &lines, bool blankLineBreaksStatements)
{
    QList<KTextEditor::Range> results;

    QueryState tstate;

    int stmtStartLine = -1;
    int stmtStartCol = -1;

    auto finalizeStatement = [&](int endLine, int endCol) {
        if (stmtStartLine >= 0) {
            results.append(KTextEditor::Range(KTextEditor::Cursor(stmtStartLine, stmtStartCol), KTextEditor::Cursor(endLine, endCol)));
            stmtStartLine = -1;
            stmtStartCol = -1;
        }
    };

    for (int lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        const QString &lineText = lines[lineIdx];
        const QChar *d = lineText.constData();
        const int len = static_cast<int>(lineText.length());

        tstate.inLineComment = false;

        // Blank line acts as statement boundary (same rule as scanTokens)
        if (blankLineBreaksStatements && tstate.parenDepth == 0 && !tstate.inBlockComment && !tstate.inString && stmtStartLine >= 0 && lineIdx > 0
            && isBlankLine(lineText)) {
            int endCol = lastNonSpaceCol(lines[lineIdx - 1]);
            if (endCol > 0) {
                finalizeStatement(lineIdx - 1, endCol);
            }
            continue;
        }

        for (int col = 0; col < len; ++col) {
            QChar c = d[col];
            QChar next = (col + 1 < len) ? d[col + 1] : QChar();

            CurrentCharacterAction action = processTokenChar(c, next, tstate);

            if (action == CurrentCharacterAction::SkipNext) {
                ++col;
                continue;
            }

            if (action == CurrentCharacterAction::BreakLine) {
                break;
            }

            if (action == CurrentCharacterAction::ParenOpen || action == CurrentCharacterAction::ParenClose) {
                continue;
            }

            if (action == CurrentCharacterAction::StatementEnd) {
                finalizeStatement(lineIdx, col);
                continue;
            }

            // CurrentTokenState::Continue — handle non-space tracking.
            // Skip characters inside block comments and line comments so
            // they don't poison statement boundaries (e.g. MySQL conditional
            // comments like /*!40101 SET ... */).
            if (!c.isSpace() && !tstate.inBlockComment && !tstate.inLineComment && stmtStartLine < 0) {
                stmtStartLine = lineIdx;
                stmtStartCol = col;
            }
        }
    }

    // Handle last statement without trailing semicolon/blank line
    if (stmtStartLine >= 0) {
        for (int line = lines.size() - 1; line >= stmtStartLine; --line) {
            int endCol = lastNonSpaceCol(lines[line]);
            if (endCol > 0) {
                finalizeStatement(line, endCol);
                break;
            }
        }
    }

    return results;
}

QStringList SQLQueryScannerStateMachine::splitStatements(const QString &text, bool blankLineBreaksStatements)
{
    QStringList lines = text.split(QLatin1Char('\n'));

    if (lines.isEmpty()) {
        return {};
    }
    const bool usesCRLF = lines[0].contains(QLatin1Char('\r'));

    if (usesCRLF) {
        for (auto &line : lines) {
            if (line.endsWith(QLatin1Char('\r')))
                line.chop(1);
        }
    }
    const QList<KTextEditor::Range> ranges = scanAllStatements(lines, blankLineBreaksStatements);

    // Build line-start offsets directly from the original text.
    // This correctly handles mixed \r\n / \n line endings.
    QVector<int> lineStarts;
    lineStarts.reserve(lines.size());
    lineStarts.append(0);
    for (int i = 0; i < text.size(); ++i) {
        if (text[i] == QLatin1Char('\n')) {
            lineStarts.append(i + 1);
        }
    }
    // If the text doesn't end with \n, lines may have one extra empty entry
    // from the split, but scanAllStatements won't produce ranges in it.

    QStringList parts(ranges.size());
    for (int i = 0; i < ranges.size(); ++i) {
        const auto &r = ranges[i];
        int start = lineStarts[r.start().line()] + r.start().column();
        int end = lineStarts[r.end().line()] + r.end().column();
        parts[i] = text.mid(start, end - start);
    }
    return parts;
}

SQLQueryScannerStateMachine::ChunkedScanState
SQLQueryScannerStateMachine::createChunkedScanState(KTextEditor::Document *doc, bool blankLineBreaksStatements, KTextEditor::Range range)
{
    ChunkedScanState state;
    state.blankLineBreaksStatements = blankLineBreaksStatements;

    if (!doc) {
        state.done = true;
        return state;
    }

    const int totalDocLines = doc->lines();
    if (totalDocLines == 0) {
        state.done = true;
        return state;
    }

    const int startLine = range.isValid() ? qBound(0, range.start().line(), totalDocLines - 1) : 0;
    state.firstLineStartCol = range.isValid() ? range.start().column() : 0;
    state.endLine = range.isValid() ? qBound(0, range.end().line(), totalDocLines - 1) : totalDocLines - 1;

    if (startLine > state.endLine) {
        state.done = true;
        return state;
    }

    state.currentLine = startLine;
    state.firstLine = startLine;
    state.rangeEndColValid = range.isValid();
    state.rangeEndCol = range.isValid() ? range.end().column() : 0;

    return state;
}

void SQLQueryScannerStateMachine::scanStatementsChunk(KTextEditor::Document *doc, ChunkedScanState &state, int maxLines, QVector<KTextEditor::Range> &outRanges)
{
    if (!doc || state.done) {
        return;
    }

    auto finalizeStatement = [&](int eLine, int eCol) {
        if (state.stmtStartLine >= 0) {
            outRanges.append(KTextEditor::Range(KTextEditor::Cursor(state.stmtStartLine, state.stmtStartCol), KTextEditor::Cursor(eLine, eCol)));
            state.stmtStartLine = -1;
            state.stmtStartCol = -1;
        }
    };

    const int lineLimit = state.currentLine + maxLines;

    for (; state.currentLine <= state.endLine && state.currentLine < lineLimit; ++state.currentLine) {
        const int lineIdx = state.currentLine;
        const QString lineText = doc->line(lineIdx);
        const QChar *d = lineText.constData();
        const int len = static_cast<int>(lineText.length());

        state.tstate.inLineComment = false;

        // Column bounds: first line starts at firstLineStartCol, last line ends at range end
        const int colStart = (lineIdx == state.firstLine) ? qMin(state.firstLineStartCol, len) : 0;
        const int colEnd = (lineIdx == state.endLine && state.rangeEndColValid) ? qMin(state.rangeEndCol, len) : len;

        // Blank line acts as statement boundary (same rule as scanAllStatements)
        // Only check for lines where we scan from column 0 (full line)
        if (colStart == 0 && state.blankLineBreaksStatements && state.tstate.parenDepth == 0 && !state.tstate.inBlockComment && !state.tstate.inString
            && state.stmtStartLine >= 0 && lineIdx > state.firstLine && isBlankLine(lineText)) {
            int prevEndCol = lastNonSpaceCol(doc->line(lineIdx - 1));
            if (prevEndCol > 0) {
                finalizeStatement(lineIdx - 1, prevEndCol);
            }
            continue;
        }

        for (int col = colStart; col < colEnd; ++col) {
            QChar c = d[col];
            QChar next = (col + 1 < len) ? d[col + 1] : QChar();

            CurrentCharacterAction action = processTokenChar(c, next, state.tstate);

            if (action == CurrentCharacterAction::SkipNext) {
                ++col;
                continue;
            }

            if (action == CurrentCharacterAction::BreakLine) {
                break;
            }

            if (action == CurrentCharacterAction::ParenOpen || action == CurrentCharacterAction::ParenClose) {
                continue;
            }

            if (action == CurrentCharacterAction::StatementEnd) {
                finalizeStatement(lineIdx, col);
                continue;
            }

            // Continue — handle non-space tracking.
            // Skip characters inside block comments and line comments so
            // they don't poison statement boundaries.
            if (!c.isSpace() && !state.tstate.inBlockComment && !state.tstate.inLineComment && state.stmtStartLine < 0) {
                state.stmtStartLine = lineIdx;
                state.stmtStartCol = col;
            }
        }
    }

    // If we've processed all lines, finalize any trailing statement
    if (state.currentLine > state.endLine) {
        state.done = true;
        if (state.stmtStartLine >= 0) {
            for (int line = state.endLine; line >= state.stmtStartLine; --line) {
                int lastCol = lastNonSpaceCol(doc->line(line));
                if (line == state.endLine && state.rangeEndColValid) {
                    lastCol = qMin(lastCol, state.rangeEndCol);
                }
                if (lastCol > 0) {
                    finalizeStatement(line, lastCol);
                    break;
                }
            }
        }
    }
}

void SQLQueryScannerStateMachine::scanAndExecuteStatements(KTextEditor::Document *doc,
                                                           bool blankLineBreaksStatements,
                                                           const std::function<bool(const KTextEditor::Range &range)> &executor,
                                                           KTextEditor::Range range)
{
    if (!doc || !executor) {
        return;
    }

    ChunkedScanState state = createChunkedScanState(doc, blankLineBreaksStatements, range);
    if (state.done) {
        return;
    }

    QVector<KTextEditor::Range> ranges;
    while (!state.done) {
        scanStatementsChunk(doc, state, 16384, ranges);
        for (const auto &r : ranges) {
            if (!executor(r)) {
                return;
            }
        }
        ranges.clear();
    }
}
