/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "sqlquerykeywordmatcher.h"

#include <KTextEditor/Cursor>
#include <KTextEditor/Range>

#include <QLatin1String>
#include <QList>
#include <QStringList>
#include <QStringView>
#include <QVector>

#include <functional>

class SQLQueryScannerStateMachine
{
protected:
    enum CurrentCharacterAction {
        Continue, // character consumed, move on
        BreakLine, // rest of line is comment (line comment started)
        SkipNext, // consumed current and next char (e.g. */ or --)
        StatementEnd, // semicolon found at parenDepth 0
        ParenOpen, // '(' found
        ParenClose, // ')' found
    };

    struct QueryState {
        bool inString = false;
        QChar stringChar;
        bool inBlockComment = false;
        bool inLineComment = false;
        int parenDepth = 0;
    };

    struct ParenthesisInfo {
        int line;
        int col;
        SQLQueryKeywordMatcher keyword;
    };

    struct ParenPair {
        KTextEditor::Cursor open;
        KTextEditor::Cursor close;
    };

    struct StmtTracker {
        int startLine = -1;
        int startCol = -1;
        SQLQueryKeywordMatcher keyword;

        void reset()
        {
            startLine = -1;
            startCol = -1;
            keyword.reset();
        }
    };

    /// Process one character of SQL token scanning.
    /// Updates `state` in-place and returns the action for the caller.
    static CurrentCharacterAction processTokenChar(QChar c, QChar next, QueryState &state);

    static inline bool isBlankLine(QStringView text);
    static inline int lastNonSpaceCol(QStringView text);
    static bool containsUnquotedSemicolon(QStringView text);

    /// Find whitespace-trimmed content between `open` (exclusive) and `close`
    /// (exclusive) cursors. Returns an invalid range if only whitespace is found.
    static KTextEditor::Range trimmedContentRange(KTextEditor::Document *doc, const KTextEditor::Cursor &open, const KTextEditor::Cursor &close);

public:
    using FirstKeyword = SQLQueryKeywordMatcher::FirstKeyword;

    struct ScanResult {
        QVector<ParenPair> selectParens;
        KTextEditor::Range enclosingStatementRange;
        FirstKeyword stmtFirstKeyword = FirstKeyword::None;
        bool foundEnclosing = false;
    };

    /// Scan SQL tokens in the given line/column bounds.
    ///
    /// Always collects paren pairs whose content starts with SELECT and that
    /// enclose `cursor` into `result.selectParens` (innermost-first).
    ///
    /// If `trackStatements` is true, also detects statement boundaries via
    /// semicolons and fills `result.enclosingStatementRange` /
    /// `result.stmtFirstKeyword` for the statement enclosing the cursor.
    ///
    /// `earlyTerminateLine` (>= 0) enables an early exit when the scanner is
    /// well past that line with no active statement.
    static ScanResult scanTokens(KTextEditor::Document *doc,
                                 KTextEditor::Cursor cursor,
                                 int startLine,
                                 int startCol,
                                 int endLine,
                                 int endCol,
                                 bool trackStatements,
                                 int earlyTerminateLine = -1,
                                 bool blankLineBreaksStatements = true);
    /// Scan all statement ranges in the given lines using the same boundary
    /// rules as scanTokens (semicolons and blank lines).
    static QList<KTextEditor::Range> scanAllStatements(const QStringList &lines, bool blankLineBreaksStatements = true);

    /// Split a SQL text into individual statements on semicolons
    /// and blank lines, respecting string literals (with escaped quotes),
    /// block comments (/* */), line comments (--), and parentheses.
    /// Reuses the same boundary rules as scanTokens.
    static QStringList splitStatements(const QString &text, bool blankLineBreaksStatements = true);

    /// Scan backward from the cursor to find the nearest line containing a raw ';'.
    /// Looks at most X lines back  (configured via KateSQLConstants::Config::DefaultValues::MaxLinesAroundTheCursorToScanForHighlight).
    /// Returns the line number (or 0 if none found).
    static int findSearchStartLine(KTextEditor::Document *doc, int cursorLine, int cursorColumn, bool blankLineBreaksStatements = true);

    /// Scan forward from (sLine, sCol), skipping whitespace and block/line
    /// comments, and return the position of the first non-whitespace /
    /// non-comment character. Returns invalid cursor if none found up to maxLine.
    static KTextEditor::Cursor skipWhitespaceAndComments(KTextEditor::Document *doc, int sLine, int sCol, int maxLine);

    /// If the text at (line, col) matches `keyword` (case-insensitive,
    /// word-boundary check), skip past it and any trailing whitespace/comments.
    /// Returns the cursor after the keyword + whitespace, or invalid if no match.
    static KTextEditor::Cursor trySkipKeyword(KTextEditor::Document *doc, int line, int col, QLatin1String keyword, int maxLine);

    /// If '(' is at (line, col), scan forward to find the matching ')'
    /// (respecting nesting depth), then skip past it and trailing
    /// whitespace/comments.  Returns invalid cursor if no '(' at position
    /// or no matching ')' found up to maxLine.
    static KTextEditor::Cursor trySkipParenthesizedOptions(KTextEditor::Document *doc, int line, int col, int maxLine);

    /// Build trimmed content ranges from a list of paren pairs.
    static QList<KTextEditor::Range> buildTrimmedRanges(KTextEditor::Document *doc, const QVector<ParenPair> &selectParens);

    /// Scan statements directly from a KTextEditor document, line by line,
    /// and invoke the executor callback for each detected statement.
    ///
    /// This avoids loading the entire document text into a single QString,
    /// making it suitable for very large files (e.g. multi-GB mysqldumps).
    ///
    /// The executor receives each statement's raw text (untrimmed) and should
    /// return true to continue scanning or false to stop early.
    static void scanAndExecuteStatements(KTextEditor::Document *doc,
                                         bool blankLineBreaksStatements,
                                         const std::function<bool(const QString &statement)> &executor,
                                         KTextEditor::Range range = KTextEditor::Range());
};
