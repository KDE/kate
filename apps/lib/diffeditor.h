#ifndef KATE_DIFF_EDITOR
#define KATE_DIFF_EDITOR

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

class DiffEditor : public QPlainTextEdit
{
    Q_OBJECT
    friend class LineNumArea;

public:
    DiffEditor(QWidget *parent = nullptr);

    void clearData()
    {
        m_data.clear();
    }
    void appendData(const QVector<LineHighlight> &newData)
    {
        m_data.append(newData);
    }

    void setLineNumberData(QVector<int> data, int maxLineNum);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *e) override;

private:
    const LineHighlight *dataForLine(int line);
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateLineNumAreaGeometry();
    void updateLineNumberAreaWidth(int);
    void updateDiffColors(bool darkMode);

    QVector<LineHighlight> m_data;
    QColor red1;
    QColor red2;
    QColor green1;
    QColor green2;
    class LineNumArea *const m_lineNumArea;
};

#endif
