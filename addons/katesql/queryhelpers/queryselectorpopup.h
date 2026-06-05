/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KTextEditor/Range>

#include <QFrame>
#include <QList>
#include <QPointer>

#include <functional>

namespace KTextEditor
{
class View;
}

class SQLQueryHighlighter;

class QCloseEvent;
class QListView;
class QStringListModel;

/// A popup widget listing query ranges for the user to pick one.
/// Supports preview highlighting and keyboard navigation.
class QuerySelectorPopup : public QFrame
{
public:
    /// Callback type invoked when the user selects a query to run.
    /// Receives the query range and the connection name.
    /// When isEntireDocument is true, range is invalid and the callback should
    /// use a streaming approach to execute the entire document.
    using QueryRunCallback = std::function<void(const KTextEditor::Range &range, const QString &connection, bool isEntireDocument)>;

    static void show(KTextEditor::View *view,
                     const QList<KTextEditor::Range> &queryRanges,
                     const QString &connection,
                     SQLQueryHighlighter *highlighter,
                     const QueryRunCallback &runCallback);

private:
    QuerySelectorPopup(KTextEditor::View *view,
                       const QList<KTextEditor::Range> &queryRanges,
                       const QString &connection,
                       SQLQueryHighlighter *highlighter,
                       const QueryRunCallback &runCallback);

    void buildModel(const QList<KTextEditor::Range> &queryRanges);
    void applyStyle();
    void positionPopup(KTextEditor::View *view);
    void connectSignals();
    void updatePreview(int row);
    void acceptSelection();
    void rejectPopup();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    /// For complicated queries, it is incredibly useful for users
    /// to have an ability to check the results of any subquery
    ///
    /// For example query like:
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
    /// (the query ranges are always ordered by "how close to the cursor the query is")
    ///
    /// This list is used for popup that allows users to select "which query to run?"
    ///
    /// P.S. Cases when there are only 1 element in the list should not even get to this popup class
    /// it should just run the query directly without initiating this class
    QList<KTextEditor::Range> m_queryRanges;

    QListView *const m_listView = nullptr;
    QStringListModel *const m_model = nullptr;
    KTextEditor::View *const m_view = nullptr;
    QPointer<SQLQueryHighlighter> m_highlighter = nullptr;

    QString m_connection;
    QueryRunCallback m_runSelectedQueryCallback;
    int m_previewIndex = -1;
};
