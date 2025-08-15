#pragma once

#include <QAbstractTableModel>
#include <QFont>

/**
 * Corresponding source code location
 */
struct SourcePos {
    QString file;
    int line = 0;
    int col = 0;
};

inline size_t qHash(const SourcePos &key, size_t seed = 0)
{
    return qHashMulti(seed, key.line, key.file);
}

inline bool operator==(const SourcePos &l, const SourcePos &r)
{
    return r.file == l.file && r.line == l.line;
}

struct LabelInRow {
    /**
     * position of label in text
     */
    int col = 0;
    int len = 0;
};

struct AsmRow {
    QList<LabelInRow> labels;

    // Corresponding source location
    // for this asm row
    SourcePos source;

    QString text;
};

class AsmViewModel : public QAbstractTableModel
{
public:
    explicit AsmViewModel(QObject *parent);

    enum Columns {
        Column_LineNo = 0,
        Column_Text = 1,
        Column_COUNT,
    };
    enum Roles {
        RowLabels = Qt::UserRole + 1,
    };

    int rowCount(const QModelIndex &p) const override
    {
        return p.isValid() ? 0 : int(m_rows.size());
    }

    int columnCount(const QModelIndex &) const override
    {
        return Column_COUNT;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    using CodeGenLines = std::vector<int>;
    void setDataFromCE(std::vector<AsmRow> text, QHash<SourcePos, CodeGenLines> sourceToAsm, QHash<QString, int> labelToAsmLines);

    void clear();

    CodeGenLines asmForSourcePos(const SourcePos &p) const
    {
        return m_sourceToAsm.value(p);
    }

    void setFont(const QFont &f)
    {
        m_font = f;
    }

    QFont font() const
    {
        return m_font;
    }

    int sourceLineForAsmLine(const QModelIndex &asmLineIndex)
    {
        if (!asmLineIndex.isValid()) {
            return -1;
        }

        int row = asmLineIndex.row();
        return m_rows.at(row).source.line;
    }

    void highlightLinkedAsm(int line);

    int asmLineForLabel(const QString &label) const
    {
        return m_labelToAsmLine.value(label, -1);
    }

    bool hasError() const
    {
        return m_hasError;
    }

    void setHasError(bool err)
    {
        m_hasError = err;
    }

private:
    std::vector<AsmRow> m_rows;
    QHash<SourcePos, CodeGenLines> m_sourceToAsm;
    QHash<QString, int> m_labelToAsmLine;
    QFont m_font;
    int m_hoveredLine = -1;
    bool m_hasError = false;
};
