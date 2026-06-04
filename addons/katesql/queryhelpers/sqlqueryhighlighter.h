/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KTextEditor/Attribute>
#include <KTextEditor/Range>

namespace KTextEditor
{
class Document;
class MainWindow;
class MovingRange;
class View;
}

#include <QList>
#include <QPointer>
#include <QTimer>

#include <memory>

class SQLQueryHighlighter : public QObject
{
public:
    explicit SQLQueryHighlighter(KTextEditor::MainWindow *mainWindow, QObject *parent = nullptr);
    ~SQLQueryHighlighter() override;

    void setup();
    void tearDown();

    /// For complicated queries, it is incredibly useful for users
    /// to have an ability to check the results of any subquery
    ///
    /// For example, query like:
    ///
    /// SELECT user_id, count(*)
    /// FROM posts
    /// WHERE user_id IN (SELECT user_id FROM users WHERE team_id = 20)
    /// GROUP BY user_id
    ///
    ///
    /// Will be stored as a list of 2 ranges
    /// Position 0: A range for `SELECT user_id FROM users WHERE team_id = 20`
    /// Position 1: A range for the full query
    /// (the query ranges should always be ordered by "how close to the cursor the query is")
    ///
    /// This list is used for popup that allows users to select "which query to run?"
    ///
    /// P.S. Cases when there are only 1 element in the list should run the query directly
    /// without initiating showing any popup
    static QList<KTextEditor::Range> findCurrentQueryRanges(KTextEditor::Document *doc, KTextEditor::Cursor cursor, bool blankLineBreaksStatements);

    /// get cached result of findCurrentQueryRanges method
    QList<KTextEditor::Range> currentQueryRanges() const;

    static bool isSqlDocument(KTextEditor::Document *doc);
    static bool isEnabledInConfig();
    bool isBlankLineBreakEnabled() const;

    // Temporarily highlight a specific range (used by query selector popup)
    // Pass invalid range to restore the default (outermost) highlight
    void setPreviewRange(const KTextEditor::Range &range);
    // Clear the preview highlight entirely without restoring the default.
    // Used when the popup shows a non-range option (e.g. "Entire document").
    void clearPreview();

    void updateConfigCache();

private:
    void slotViewChanged(KTextEditor::View *view);
    void slotCursorPositionChanged(KTextEditor::View *view);
    void slotTextChanged();
    void slotDocumentUrlChanged(KTextEditor::Document *doc);
    void updateQueryHighlight();
    bool tryIncrementalCacheUpdate(KTextEditor::Document *doc, KTextEditor::Cursor cursor);
    void applyHighlightRange(KTextEditor::Document *doc, KTextEditor::Range range);
    void clearQueryHighlight();

    /// get cached result of findCurrentQueryRanges method
    QList<KTextEditor::Range> m_cachedQueryRanges;

    KTextEditor::MainWindow *m_mainWindow;

    bool m_blankLineBreaksStatements = true;

    QPointer<KTextEditor::View> m_activeView;
    std::unique_ptr<KTextEditor::MovingRange> m_queryHighlightRange;
    KTextEditor::Attribute::Ptr m_queryHighlightAttribute;
    QTimer m_queryHighlightDebouncingTimer;
    QTimer m_initialHighlightTimer;
};
