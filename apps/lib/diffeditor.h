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

enum DiffStyle { SideBySide, Unified, Raw };

class DiffEditor : public QPlainTextEdit
{
    Q_OBJECT
    friend class LineNumArea;

public:
    DiffEditor(QWidget *parent = nullptr);

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

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    const LineHighlight *dataForLine(int line);
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateLineNumAreaGeometry();
    void updateLineNumberAreaWidth(int);
    void updateDiffColors(bool darkMode);
    void onContextMenuRequest();

    QVector<LineHighlight> m_data;
    QColor red1;
    QColor red2;
    QColor green1;
    QColor green2;
    class LineNumArea *const m_lineNumArea;

Q_SIGNALS:
    void switchStyle(int);
};

#endif
