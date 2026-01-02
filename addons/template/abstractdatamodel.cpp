// SPDX-FileCopyrightText: 2024 Kåre Särs <kare.sars@iki.fi>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "abstractdatamodel.h"

using namespace Qt::Literals::StringLiterals;

// Default implementation for the AbstractData
Qt::ItemFlags AbstractData::flags(int) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool AbstractData::setData(const QVariant &, int, int)
{
    return false;
}

QHash<int, QByteArray> AbstractData::roleNames() const
{
    return {};
}

AbstractDataModel::TreeNode::TreeNode(std::unique_ptr<AbstractData> data, TreeNode *parent)
    : m_data(std::move(data))
    , m_parent(parent)
{
}

int AbstractDataModel::TreeNode::rowInParent() const
{
    if (m_parent == nullptr) {
        return 0;
    }

    for (size_t i = 0; i < m_parent->m_children.size(); ++i) {
        if (m_parent->m_children[i].get() == this) {
            return (int)i;
        }
    }
    return -1;
}

AbstractDataModel::AbstractDataModel(std::unique_ptr<AbstractData> headerData, QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootNode(std::move(headerData), nullptr)
    , m_roleNames(QAbstractItemModel::roleNames())
{
    m_roleNames.insert(m_rootNode.m_data->roleNames());
}

QVariant AbstractDataModel::headerData(int column, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    if (!m_rootNode.m_data) {
        return {};
    }
    return m_rootNode.m_data->data(role, column);
}

QVariant AbstractDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const auto *node = static_cast<const TreeNode *>(index.internalPointer());
    if (node == nullptr) {
        return {};
    }

    return node->m_data->data(role, index.column());
}

int AbstractDataModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return (int)m_rootNode.m_children.size();
    }

    if (parent.column() != 0) {
        return 0;
    }

    const TreeNode *pNode = static_cast<TreeNode *>(parent.internalPointer());
    if (pNode) {
        return (int)pNode->m_children.size();
    }

    return 0;
}

int AbstractDataModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_rootNode.m_data->columns();
    }
    const TreeNode *pNode = static_cast<TreeNode *>(parent.internalPointer());
    if (pNode) {
        return pNode->m_data->columns();
    }
    return 1;
}

QModelIndex AbstractDataModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return {};
    }
    const TreeNode *pNode = parent.isValid() ? static_cast<TreeNode *>(parent.internalPointer()) : &m_rootNode;
    if (row < 0 || row >= (int)pNode->m_children.size()) {
        return {};
    }
    void *child = pNode->m_children[row].get();

    return createIndex(row, column, child);
}

QModelIndex AbstractDataModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return {};
    }
    TreeNode *cNode = static_cast<TreeNode *>(child.internalPointer());
    if (!cNode) {
        return {};
    }

    if (cNode->m_parent == &m_rootNode) {
        return {};
    }
    TreeNode *pNode = cNode->m_parent;
    return createIndex(pNode->rowInParent(), 0, pNode);
}

QModelIndex AbstractDataModel::addChild(std::unique_ptr<AbstractData> data, const QModelIndex &parent)
{
    TreeNode *pNode = static_cast<TreeNode *>(parent.internalPointer());
    if (!pNode) {
        pNode = &m_rootNode;
    }
    int row = (int)pNode->m_children.size();
    beginInsertRows(parent, row, row);
    pNode->m_children.push_back(std::make_unique<TreeNode>(std::move(data), pNode));
    endInsertRows();

    return index(row, 0, parent);
}

bool AbstractDataModel::setAbstractData(std::unique_ptr<AbstractData> data, const QModelIndex &mIndex)
{
    if (!mIndex.isValid()) {
        return false;
    }

    TreeNode *node = static_cast<TreeNode *>(mIndex.internalPointer());
    node->m_data.swap(data);
    dataChanged(mIndex, mIndex);
    return true;
}

QHash<int, QByteArray> AbstractDataModel::roleNames() const
{
    return m_roleNames;
}

Qt::ItemFlags AbstractDataModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    TreeNode *node = static_cast<TreeNode *>(index.internalPointer());
    if (!node) {
        return Qt::NoItemFlags;
    }

    return node->m_data->flags(index.column());
}

bool AbstractDataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }
    TreeNode *node = static_cast<TreeNode *>(index.internalPointer());
    if (!node) {
        return false;
    }
    bool ok = node->m_data->setData(value, role, index.column());
    if (ok) {
        dataChanged(index, index, {role});
    }
    return ok;
}

void AbstractDataModel::clear()
{
    beginResetModel();
    m_rootNode.m_children.clear();
    endResetModel();
}

bool AbstractDataModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (orientation != Qt::Horizontal) {
        return false;
    }
    return m_rootNode.m_data->setData(value, role, section);
}

#include "moc_abstractdatamodel.cpp"
