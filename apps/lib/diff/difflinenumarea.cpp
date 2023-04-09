/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "difflinenumarea.h"
#include "diffeditor.h"

#include <QPainter>

#include <KTextEditor/Editor>

static constexpr int Margin = 4;

struct LineNumColors {
    const QPen &otherLine;
    //     const QPen &currentLine;
    const QPen &added;
    const QPen &removed;
};

LineNumArea::LineNumArea(DiffEditor *parent)
    : QWidget(parent)
    , textEdit(parent)
{
    setFont(textEdit->font());
    auto updateColors = [this](KTextEditor::Editor *e) {
        if (!e)
            return;
        auto theme = e->theme();
        //         m_currentLineBgColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::CurrentLine));
        //         m_currentLineColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::CurrentLineNumber));
        m_otherLinesColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::LineNumbers));
        m_borderColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::Separator));
        auto bg = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::IconBorder));
        auto pal = palette();
        pal.setColor(QPalette::Window, bg);
        setPalette(pal);
    };
    connect(KTextEditor::Editor::instance(), &KTextEditor::Editor::configChanged, this, updateColors);
    updateColors(KTextEditor::Editor::instance());
}

int LineNumArea::lineNumAreaWidth() const
{
    int digits = 1;
    int max = std::max(1, maxLineNum);
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 13 + textEdit->fontMetrics().horizontalAdvance(u'9') * digits;
}

void LineNumArea::setLineNumData(QVector<int> leftLineNos, QVector<int> rightLineNos)
{
    m_lineToNumA = std::move(leftLineNos);
    m_lineToNumB = std::move(rightLineNos);
}

QSize LineNumArea::sizeHint() const
{
    int lineNosWidth = lineNumAreaWidth();
    if (!m_lineToNumB.empty()) {
        lineNosWidth *= 2;
    }
    return {lineNosWidth + textEdit->fontMetrics().height(), 0};
}

static void paintTriangle(QPainter &painter, QColor c, int xOffset, int yOffset, int width, int height, bool open)
{
    painter.setRenderHint(QPainter::Antialiasing);

    qreal size = qMin(width, height);

    QPen pen;
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setColor(c);
    pen.setWidthF(1.5);
    painter.setPen(pen);
    painter.setBrush(c);

    // let some border, if possible
    size *= 0.6;

    qreal halfSize = size / 2;
    qreal halfSizeP = halfSize * 0.6;
    QPointF middle(xOffset + (qreal)width / 2, yOffset + (qreal)height / 2);

    if (open) {
        QPointF points[3] = {middle + QPointF(-halfSize, -halfSizeP), middle + QPointF(halfSize, -halfSizeP), middle + QPointF(0, halfSizeP)};
        painter.drawConvexPolygon(points, 3);
    } else {
        QPointF points[3] = {middle + QPointF(-halfSizeP, -halfSize), middle + QPointF(-halfSizeP, halfSize), middle + QPointF(halfSizeP, 0)};
        painter.drawConvexPolygon(points, 3);
    }

    painter.setRenderHint(QPainter::Antialiasing, false);
}

void LineNumArea::paintEvent(QPaintEvent *event)
{
    if (m_lineToNumA.isEmpty()) {
        return;
    }
    QPainter painter(this);

    painter.fillRect(event->rect(), palette().color(QPalette::Active, QPalette::Window));

    auto block = textEdit->firstVisibleBlock();
    int blockNumber = block.blockNumber();
    qreal top = textEdit->blockBoundingGeometry(block).translated(textEdit->contentOffset()).top();
    // Maybe the top is not 0?
    top += textEdit->viewportMargins().top();
    qreal bottom = top;

    //     const QPen currentLine = m_currentLineColor;
    const QPen otherLines = m_otherLinesColor;

    QColor c = textEdit->addedColor();
    c.setAlphaF(0.6);
    const QPen added = c;
    c = textEdit->removedColor();
    c.setAlphaF(0.6);
    const QPen removed = c;

    LineNumColors colors{otherLines /*, currentLine*/, added, removed};

    painter.setFont(font());
    const int w = lineNumAreaWidth();

    while (block.isValid() && top <= event->rect().bottom()) {
        top = bottom;
        bottom = top + textEdit->blockBoundingRect(block).height();
        if (block.isVisible() && bottom >= event->rect().top()) {
            // Current line background
            //             const auto isCurrentLine = textEdit->textCursor().blockNumber() == blockNumber;
            //             if (isCurrentLine) {
            //                 painter.fillRect(0, top, rect().width(), textEdit->fontMetrics().height(), m_currentLineBgColor);
            //             }

            // Line number left
            int n = m_lineToNumA.value(blockNumber, -1);
            QRect numRect(0, top, w, textEdit->fontMetrics().height());
            drawLineNumber(painter, numRect, blockNumber, n, colors);

            if (!m_lineToNumB.empty()) {
                // we are in unified mode, draw the right line number
                int n = m_lineToNumB.value(blockNumber, -1);
                QRect numRect(w, top, w, textEdit->fontMetrics().height());
                drawLineNumber(painter, numRect, blockNumber, n, colors);
            }

            // Hunk fold marker
            if (textEdit->isHunkLine(blockNumber)) {
                const int x = rect().width() - (textEdit->fontMetrics().height() + Margin);
                const int y = top;
                const int width = textEdit->fontMetrics().height();
                const int height = width;
                paintTriangle(painter, m_otherLinesColor, x, y, width, height, !textEdit->isHunkFolded(blockNumber));
            }
        }

        block = block.next();
        ++blockNumber;
    }

    // draw the line num area border
    if (m_lineToNumB.empty()) {
        // side by side
        painter.setPen(m_borderColor);
        painter.drawLine(rect().topRight() - QPoint(1, 0), rect().bottomRight() - QPoint(1, 0));
    } else {
        // unified
        painter.setPen(m_borderColor);
        painter.drawLine(QPoint(w, 0) - QPoint(1, 0), QPoint(w, rect().bottom()) - QPoint(1, 0));
        painter.setPen(m_borderColor);
        painter.drawLine(rect().topRight() - QPoint(1, 0), rect().bottomRight() - QPoint(1, 0));
    }
}

void LineNumArea::drawLineNumber(QPainter &painter, QRect rect, int blockNumber, int num, const LineNumColors &c)
{
    if (num < 0) {
        return;
    }

    const QString number = QString::number(num);
    QPen p = c.otherLine;
    auto hl = textEdit->highlightingForLine(blockNumber);
    if (hl) {
        p = hl->added ? c.added : c.removed;
    }

    painter.setPen(p);
    rect.adjust(0, 0, -(Margin * 2), 0);
    painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter | Qt::TextDontClip, number);
}

void LineNumArea::mousePressEvent(QMouseEvent *e)
{
    auto pos = e->pos();
    auto r = rect();
    r.setLeft(lineNumAreaWidth());
    if (!r.contains(pos)) {
        QWidget::mousePressEvent(e);
        return;
    }

    int block = textEdit->cursorForPosition(pos).block().blockNumber();
    if (textEdit->isHunkLine(block)) {
        textEdit->toggleFoldHunk(block);
    }
}
