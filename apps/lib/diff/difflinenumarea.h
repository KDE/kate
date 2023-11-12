/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QWidget>

class LineNumArea final : public QWidget
{
    Q_OBJECT
public:
    explicit LineNumArea(class DiffEditor *parent);

    int lineNumAreaWidth() const;
    QSize sizeHint() const override;

    void setLineNumData(std::vector<int> leftLineNos, std::vector<int> rightLineNos);
    void setMaxLineNum(int n)
    {
        maxLineNum = n;
    }

    int lineNumForBlock(int block)
    {
        auto it = std::find(m_lineToNumA.begin(), m_lineToNumA.end(), block);
        if (it == m_lineToNumA.end()) {
            return 0;
        }
        return *it;
    }

    int blockForLineNum(int lineNo)
    {
        auto it = std::find(m_lineToNumA.begin(), m_lineToNumA.end(), lineNo);
        return std::distance(m_lineToNumA.begin(), it);
    }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    void drawLineNumber(QPainter &painter, QRect rect, int blockNumber, int num, const struct LineNumColors &c);

    class DiffEditor *const textEdit;
    //     QColor m_currentLineColor;
    //     QBrush m_currentLineBgColor;
    QColor m_otherLinesColor;
    QColor m_borderColor;
    std::vector<int> m_lineToNumA;
    std::vector<int> m_lineToNumB;
    int maxLineNum = 0;
};
