#include "AsmViewModel.h"

#include <QColor>
#include <QDebug>
#include <QFont>
#include <QFontDatabase>

AsmViewModel::AsmViewModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant AsmViewModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == Column_LineNo)
            return QString::number(row + 1);
        else if (index.column() == Column_Text)
            return m_rows.at(row).text;
        else
            Q_UNREACHABLE();
    case Qt::FontRole:
        return m_font;
    case Qt::BackgroundRole:
        if (index.column() == Column_Text) {
            if (m_hoveredLine != -1) {
                const auto &sourcePos = m_rows.at(row).source;
                if (sourcePos.file.isEmpty() && sourcePos.line == m_hoveredLine + 1) {
                    return QColor(17, 90, 130); // Dark bluish, probably need better logic and colors
                }
            }
        }
        break;
    case Roles::RowLabels:
        if (index.column() == Column_Text) {
            return QVariant::fromValue(m_rows.at(row).labels);
        }
    }
    return {};
}

void AsmViewModel::setDataFromCE(std::vector<AsmRow> text, QHash<SourcePos, CodeGenLines> sourceToAsm, QHash<QString, int> labelToAsmLines)
{
    beginResetModel();
    m_rows = std::move(text);
    endResetModel();

    m_sourceToAsm = std::move(sourceToAsm);
    m_labelToAsmLine = std::move(labelToAsmLines);
}

void AsmViewModel::clear()
{
    beginResetModel();
    m_rows.clear();
    endResetModel();
    m_sourceToAsm.clear();
}

void AsmViewModel::highlightLinkedAsm(int line)
{
    m_hoveredLine = line;
}
