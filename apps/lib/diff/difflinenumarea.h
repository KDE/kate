/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QWidget>

class LineNumArea final : public QWidget
{
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
        return block < ((int)m_lineToNumA.size()) ? m_lineToNumA[block] : 0;
    }

    int lineNumForBlockB(int block)
    {
        return block < ((int)m_lineToNumB.size()) ? m_lineToNumB[block] : 0;
    }

    /**
     * Returns the block mapped to the closest line number because
     * when switching to a diff without full context not all line numbers are available
     */
    int blockForLineNum(int lineNo)
    {
        int i = 0, index = 0;
        int distance = INT_MAX;

        for (int num : m_lineToNumA) {
            if (num > 0) { // Skip placeholder values
                int diff = abs(lineNo - num);
                if (diff < distance) {
                    index = i;
                    distance = diff;
                }
                if (distance == 0) {
                    break;
                }
            }
            i++;
        }

        return index;
    }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

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
