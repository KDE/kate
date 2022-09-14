/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef DIFF_LINE_NUM_AREA
#define DIFF_LINE_NUM_AREA

#include <QWidget>

class LineNumArea final : public QWidget
{
    Q_OBJECT
public:
    explicit LineNumArea(class DiffEditor *parent);

    int lineNumAreaWidth() const;
    QSize sizeHint() const override;

    void setLineNumData(QVector<int> leftLineNos, QVector<int> rightLineNos);
    void setMaxLineNum(int n)
    {
        maxLineNum = n;
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
    QVector<int> m_lineToNumA;
    QVector<int> m_lineToNumB;
    int maxLineNum = 0;
};

#endif
