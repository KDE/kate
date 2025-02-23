#include "TMPL_QAbstractListModel.h"

#include <QDebug>

TMPL_QAbstractListModel::TMPL_QAbstractListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

bool TMPL_QAbstractListModel::insertAt(int row, const Data &item)
{
    if (row < 0 || row > m_list.size()) {
        qWarning() << "Cant insert at invalid row:" << row << m_list.size();
        return false;
    }

    beginInsertRows(QModelIndex(), row, row);
    m_list.insert(row, item);
    endInsertRows();
    return true;
}

std::optional<TMPL_QAbstractListModel::Data> TMPL_QAbstractListModel::takeAt(int row)
{
    if (row < 0 || row >= m_list.size()) {
        qWarning() << "Cant take a card at invalid row:" << row << m_list.size();
        return std::nullopt;
    }
    beginRemoveRows(QModelIndex(), row, row);
    auto item = m_list.takeAt(row);
    endRemoveRows();
    return item;
}

bool TMPL_QAbstractListModel::removeAt(int row)
{
    if (row < 0 || row >= m_list.size()) {
        qWarning() << "Cant take a card at invalid row:" << row << m_list.size();
        return false;
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_list.removeAt(row);
    endRemoveRows();
    return true;
}

void TMPL_QAbstractListModel::clear()
{
    beginResetModel();
    m_list.clear();
    endResetModel();
}

QHash<int, QByteArray> TMPL_QAbstractListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[DataRole] = "data";
    // More roles goes here
    return roles;
}

int TMPL_QAbstractListModel::rowCount(const QModelIndex &) const
{
    return m_list.count();
}

QVariant TMPL_QAbstractListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= m_list.count() || index.row() < 0) {
        return QVariant();
    }

    const auto &item = m_list.at(index.row());
    switch (static_cast<Roles>(role)) {
    case DataRole:
        return item.data;
        // More roles handled here
    }

    return QVariant();
}
