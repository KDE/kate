// SPDX-FileCopyrightText: 2024 Kåre Särs <kare.sars@iki.fi>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QAbstractItemModel>
#include <QVariant>

struct AbstractData {
    // Mandatory implementation
    virtual QVariant data(int role = Qt::DisplayRole, int column = 0) = 0;
    virtual int columns() = 0;

    // Optional functions
    virtual Qt::ItemFlags flags(int column) const;
    virtual bool setData(const QVariant &value, int role = Qt::DisplayRole, int column = 0);

    virtual QHash<int, QByteArray> roleNames() const;
    virtual ~AbstractData()
    {
    }
};

class AbstractDataModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AbstractDataModel(std::unique_ptr<AbstractData> data, QObject *parent = nullptr);

    // Functions to populate the model
    QModelIndex addChild(std::unique_ptr<AbstractData> data, const QModelIndex &parent);
    bool setAbstractData(std::unique_ptr<AbstractData> data, const QModelIndex &mIndex);
    bool removeAt(const QModelIndex &mIndex);
    void clear();

    // Model functions that views are interested in
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

private:
    struct TreeNode {
        explicit TreeNode(std::unique_ptr<AbstractData> data, TreeNode *parent);
        int rowInParent() const;

        std::vector<std::unique_ptr<TreeNode>> m_children;
        std::unique_ptr<AbstractData> m_data;
        TreeNode *m_parent = nullptr;
    };

    TreeNode m_rootNode;
    QHash<int, QByteArray> m_roleNames;
};
