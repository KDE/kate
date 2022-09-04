/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "difflinenumarea.h"
#include "diffeditor.h"

#include <QPainter>
#include <QTextBlock>

#include <KTextEditor/Editor>
// #include <KColorUtils>

static constexpr int Margin = 4;

LineNumArea::LineNumArea(DiffEditor *parent)
    : QWidget(parent)
    , textEdit(parent)
{
    setFont(textEdit->font());
    auto updateColors = [this](KTextEditor::Editor *e) {
        if (!e)
            return;
        auto theme = e->theme();
        m_currentLineColor = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::CurrentLineNumber));
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

void LineNumArea::setLineNumData(QVector<int> data)
{
    m_lineToNum = std::move(data);
}

QSize LineNumArea::sizeHint() const
{
    return {lineNumAreaWidth() + textEdit->fontMetrics().height(), 0};
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
    if (m_lineToNum.isEmpty()) {
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

    const QPen currentLine = m_currentLineColor;
    const QPen otherLines = m_otherLinesColor;
    painter.setFont(font());

    const int w = lineNumAreaWidth();

    while (block.isValid() && top <= event->rect().bottom()) {
        top = bottom;
        bottom = top + textEdit->blockBoundingRect(block).height();
        if (block.isVisible() && bottom >= event->rect().top()) {
            int n = m_lineToNum.value(blockNumber, -1);
            if (n > -1) {
                const QString number = QString::number(n);
                auto isCurrentLine = textEdit->textCursor().blockNumber() == blockNumber;
                painter.setPen(isCurrentLine ? currentLine : otherLines);
                QRect numRect(0, top, w, textEdit->fontMetrics().height());
                numRect.adjust(0, 0, -(Margin * 2), 0);
                painter.drawText(numRect, Qt::AlignRight | Qt::AlignVCenter | Qt::TextDontClip, number);
            } else if (textEdit->isHunkLine(blockNumber)) {
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

    painter.setPen(m_borderColor);
    painter.drawLine(rect().topRight() - QPoint(1, 0), rect().bottomRight() - QPoint(1, 0));
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
