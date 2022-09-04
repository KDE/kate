/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "difflinenumarea.h"
#include "diffeditor.h"

#include <QPainter>
#include <QTextBlock>

#include <KTextEditor/Editor>

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
    int digits = 2;
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
    return {lineNumAreaWidth(), 0};
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

    while (block.isValid() && top <= event->rect().bottom()) {
        top = bottom;
        bottom = top + textEdit->blockBoundingRect(block).height();
        if (block.isVisible() && bottom >= event->rect().top()) {
            int n = m_lineToNum.value(blockNumber, -1);
            if (n > -1) {
                const QString number = QString::number(n);
                auto isCurrentLine = textEdit->textCursor().blockNumber() == blockNumber;
                painter.setPen(isCurrentLine ? currentLine : otherLines);
                painter.drawText(-5, top, sizeHint().width(), textEdit->fontMetrics().height(), Qt::AlignRight, number);
            }
        }

        block = block.next();
        ++blockNumber;
    }

    painter.setPen(m_borderColor);
    painter.drawLine(rect().topRight() - QPoint(1, 0), rect().bottomRight() - QPoint(1, 0));
}
