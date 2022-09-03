/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: MIT
*/
#ifndef KATE_DIFF_EDITOR
#define KATE_DIFF_EDITOR

#include "diffparams.h"
#include <KTextEditor/Range>
#include <QPlainTextEdit>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using IntT = qsizetype;
#else
using IntT = int;
#endif

struct Change {
    IntT pos;
    IntT len;
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
        setLineNumberData({}, 0);
    }
    void appendData(const QVector<LineHighlight> &newData)
    {
        m_data.append(newData);
    }

    void setLineNumberData(QVector<int> data, int maxLineNum);

    KTextEditor::Range selectionRange() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    const LineHighlight *highlightingForLine(int line);
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

Q_SIGNALS:
    void switchStyle(int);
    void actionTriggered(DiffEditor *e, int startLine, int endLine, int actionType, DiffParams::Flag);
};

#endif
