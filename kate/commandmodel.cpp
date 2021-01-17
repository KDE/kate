#include "commandmodel.h"

#include <QAction>
#include <KLocalizedString>
#include <QDebug>

CommandModel::CommandModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void CommandModel::refresh(QList<QAction *> actions)
{
    QVector<Item> temp;
    temp.reserve(actions.size());
    for (auto action : actions) {
        temp.push_back({action, 0});
    }

    beginResetModel();
    m_rows = std::move(temp);
    endResetModel();
}

QVariant CommandModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    auto entry = m_rows[index.row()];
    int col = index.column();

    switch (role)
    {
    case Qt::DisplayRole:
        if (col == 0)
            return KLocalizedString::removeAcceleratorMarker(entry.action->text());
        else
            return entry.action->shortcut().toString();
    case Qt::DecorationRole:
        if (col == 0)
            return entry.action->icon();
        break;
    case Qt::TextAlignmentRole:
        if (col == 0)
            return Qt::AlignLeft;
        else
            return Qt::AlignRight;
    case Qt::UserRole:
    {
        QVariant v;
        v.setValue(entry.action);
        return v;
    }
    case Role::Score:
        return entry.score;
    }

    return {};
}
