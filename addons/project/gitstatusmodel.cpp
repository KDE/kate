#include "gitstatusmodel.h"

#include <QDebug>
#include <QFont>
#include <QIcon>
#include <QMimeDatabase>

static constexpr int Staged = 0;
static constexpr int Changed = 1;
static constexpr int Conflict = 2;
static constexpr int Untrack = 3;
static constexpr quintptr Root = 0xFFFFFFFF;

GitStatusModel::GitStatusModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    // setup root rows
    beginInsertRows(QModelIndex(), 0, 3);
    endInsertRows();
}

QModelIndex GitStatusModel::index(int row, int column, const QModelIndex &parent) const
{
    auto rootIndex = Root;
    if (parent.isValid()) {
        if (parent.internalId() == Root) {
            rootIndex = parent.row();
        }
    }
    return createIndex(row, column, rootIndex);
}

QModelIndex GitStatusModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    return createIndex(child.internalId(), 0, Root);
}

int GitStatusModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return 4;
    }

    if (parent.internalId() == Root) {
        if (parent.row() < 0 || parent.row() > 3) {
            return 0;
        }

        return m_nodes[parent.row()].size();
    }
    return 0;
}

int GitStatusModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant GitStatusModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    auto row = index.row();

    if (index.internalId() == Root) {
        if (role == Qt::DisplayRole) {
            if (row == Staged) {
                return QStringLiteral("Staged");
            } else if (row == Untrack) {
                return QStringLiteral("Untracked");
            } else if (row == Conflict) {
                return QStringLiteral("Conflict");
            } else if (row == Changed) {
                return QStringLiteral("Modified");
            }
        } else if (role == Qt::FontRole) {
            QFont bold;
            bold.setBold(true);
            return bold;
        } else if (role == Qt::DecorationRole) {
            static const auto branchIcon = QIcon(QStringLiteral(":/kxmlgui5/kateproject/sc-apps-git.svg"));
            return branchIcon;
        }
    } else {
        if (role != Qt::DisplayRole && role != Qt::DecorationRole) {
            return {};
        }
        int rootIndex = index.internalId();
        if (rootIndex < 0 || rootIndex > 3) {
            return QVariant();
        }

        if (role == Qt::DisplayRole) {
            return m_nodes[rootIndex].at(row).file;
        } else if (role == Qt::DecorationRole) {
            return QIcon::fromTheme(QMimeDatabase().mimeTypeForFile(m_nodes[rootIndex].at(row).file, QMimeDatabase::MatchExtension).iconName());
        }
    }

    return {};
}
void GitStatusModel::addItems(const QVector<GitUtils::StatusItem> &staged,
                              const QVector<GitUtils::StatusItem> &changed,
                              const QVector<GitUtils::StatusItem> &unmerge,
                              const QVector<GitUtils::StatusItem> &untracked)
{
    beginResetModel();
    m_nodes[Staged] = staged;
    m_nodes[Changed] = changed;
    m_nodes[Conflict] = unmerge;
    m_nodes[Untrack] = untracked;
    endResetModel();
}

QVector<int> GitStatusModel::emptyRows()
{
    QVector<int> empty;
    for (int i = 0; i < 4; ++i) {
        if (m_nodes[i].isEmpty()) {
            empty.append(i);
        }
    }
    return empty;
}
