/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_DIFF_EDITOR
#define KATE_DIFF_EDITOR

#include "diffparams.h"
#include <KTextEditor/Range>
#include <QPlainTextEdit>

#include <KSyntaxHighlighting/FoldingRegion>
#include <KSyntaxHighlighting/SyntaxHighlighter>

#include <QTimeLine>

class DiffSyntaxHighlighter final : public KSyntaxHighlighting::SyntaxHighlighter
{
public:
    DiffSyntaxHighlighter(QTextDocument *parent, class DiffWidget *diffWidget);

    void highlightBlock(const QString &text) override;
    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) override;
    void applyFolding(int, int, KSyntaxHighlighting::FoldingRegion) override
    {
        // no folding
    }

private:
    class DiffWidget *const m_diffWidget;
};

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using IntT = qsizetype;
#else
using IntT = int;
#endif

struct Change {
    IntT pos;
    IntT len;
    enum { FullBlock = -2 };
};

struct LineHighlight {
    QVector<Change> changes;
    IntT line;
    bool added;
};

enum DiffStyle { SideBySide = 0, Unified, Raw };

class DiffEditor : public QPlainTextEdit
{
    Q_OBJECT
    friend class LineNumArea;

public:
    enum { Line = 0, Hunk = 1 };
    DiffEditor(DiffParams::Flags, QWidget *parent = nullptr);
    struct State {
        int scrollValue;
        int cursorPosition;
    };

    State saveState() const;
    void restoreState(State);

    void clearData()
    {
        clear();
        m_data.clear();
        setLineNumberData({}, {}, 0);
    }
    void appendData(const QVector<LineHighlight> &newData)
    {
        m_data.append(newData);
    }

    void setLineNumberData(QVector<int> lineNosA, QVector<int> lineNosB, int maxLineNum);

    KTextEditor::Range selectionRange() const;

    bool isHunkLine(int line) const;
    void toggleFoldHunk(int block);
    bool isHunkFolded(int blockNumber);
    const LineHighlight *highlightingForLine(int line);

    QColor removedColor() const
    {
        return red1;
    }
    QColor addedColor() const
    {
        return green1;
    }

    int firstVisibleBlockNumber() const
    {
        return QPlainTextEdit::firstVisibleBlock().blockNumber();
    }

    void scrollToBlock(int block);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateLineNumAreaGeometry();
    void updateLineNumberAreaWidth(int);
    void updateDiffColors(bool darkMode);
    void onContextMenuRequest();
    void addStageUnstageDiscardActions(QMenu *menu);

    QVector<LineHighlight> m_data;
    QColor red1;
    QColor red2;
    QColor green1;
    QColor green2;
    QColor hunkSeparatorColor;
    class LineNumArea *const m_lineNumArea;
    class DiffWidget *const m_diffWidget;
    DiffParams::Flags m_flags;

    QTimeLine m_timeLine;
    QRect m_animateTextRect;

Q_SIGNALS:
    void switchStyle(int);
    void actionTriggered(DiffEditor *e, int startLine, int endLine, int actionType, DiffParams::Flag);
};

#endif
