/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "gitstatusmodel.h"

#include <KColorScheme>
#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QMimeDatabase>

#include <KLocalizedString>

static constexpr int Staged = 0;
static constexpr int Changed = 1;
static constexpr int Conflict = 2;
static constexpr int Untrack = 3;
static constexpr quintptr Root = 0xFFFFFFFF;

static QByteArray getUniqueFilename(const QByteArray &filename, QByteArray &path)
{
    const auto i = path.lastIndexOf('/');
    if (i != -1) {
        QByteArray name = path.mid(i + 1).append('/').append(filename);
        // Remove the bit from path that we just appended
        path = path.left(i);
        return name;
    }
    return QByteArray();
}

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
    return 2;
}

QVariant GitStatusModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const int row = index.row();

    if (index.internalId() == Root) {
        if (role == Qt::DisplayRole) {
            if (index.column() == 1) {
                return QString::number(m_nodes[row].count());
            } else {
                if (row == Staged) {
                    return i18n("Staged");
                } else if (row == Untrack) {
                    return i18n("Untracked");
                } else if (row == Conflict) {
                    return i18n("Conflict");
                } else if (row == Changed) {
                    return i18n("Modified");
                } else {
                    Q_UNREACHABLE();
                }
            }
        } else if (role == Qt::FontRole) {
            QFont bold;
            bold.setBold(true);
            return bold;
        } else if (role == Role::TreeItemType) {
            return NodeStage + row;
        } else if (role == Qt::TextAlignmentRole) {
            if (index.column() == 0) {
                return (int)(Qt::AlignLeft | Qt::AlignVCenter);
            } else {
                return (int)(Qt::AlignRight | Qt::AlignVCenter);
            }
        }
    } else {
        int rootIndex = index.internalId();
        if (rootIndex < 0 || rootIndex > 3) {
            return QVariant();
        }

        if (role == Qt::DisplayRole) {
            if (index.column() == 0) {
                auto fileStr = QString::fromUtf8(m_nodes[rootIndex].at(row).file);
                QFileInfo fi(fileStr);
                const auto filename = fi.fileName();
                if (filename.isEmpty()) {
                    return m_nodes[rootIndex].at(row).file;
                }
                if (rootIndex < Untrack && m_nonUniqueFileNames.contains(filename)) {
                    QByteArray path = fi.path().toUtf8();
                    QByteArray name = getUniqueFilename(filename.toUtf8(), path);

                    // Iterate through all items in this node and ensure we are unique
                    for (int i = 0; i < m_nodes[rootIndex].size(); i++) {
                        if (i == row) {
                            continue; // skip self
                        }
                        if (name.isEmpty()) {
                            break;
                        }
                        const GitUtils::StatusItem &item = m_nodes[rootIndex].at(i);
                        // We found another item with this name, try to uniquify more
                        while (!item.file.isEmpty() && item.file.endsWith(name)) {
                            name = getUniqueFilename(name, path);
                            if (name.isEmpty()) {
                                break;
                            }
                        }
                    }

                    if (name.isEmpty()) {
                        return fileStr;
                    }
                    return QString::fromUtf8(name);
                }
                return filename;
            } else {
                int a = m_nodes[rootIndex].at(row).linesAdded;
                int r = m_nodes[rootIndex].at(row).linesRemoved;
                auto add = QString::number(a);
                auto sub = QString::number(r);
                QString statusChar(QLatin1Char(m_nodes[rootIndex].at(row).statusChar));
                QString ret = QStringLiteral("+") + add + QStringLiteral(" -") + sub + QStringLiteral(" ") + statusChar;
                return ret;
            }
        } else if (role == FileNameRole) {
            return m_nodes[rootIndex].at(row).file;
        } else if (role == Qt::DecorationRole) {
            if (index.column() == 0) {
                const QString file = QString::fromUtf8(m_nodes[rootIndex].at(row).file);
                return QIcon::fromTheme(QMimeDatabase().mimeTypeForFile(file, QMimeDatabase::MatchExtension).iconName());
            }
        } else if (role == Role::TreeItemType) {
            return ItemType::NodeFile;
        } else if (role == Qt::ToolTipRole) {
            return QString(QString::fromUtf8(m_nodes[rootIndex].at(row).file) + GitUtils::statusString(m_nodes[rootIndex].at(row).status));
        } else if (role == Qt::TextAlignmentRole) {
            if (index.column() == 0) {
                return (int)(Qt::AlignLeft | Qt::AlignVCenter);
            } else {
                return (int)(Qt::AlignRight | Qt::AlignVCenter);
            }
        } else if (role == Qt::ForegroundRole) {
            if (index.column() == 1 && rootIndex > 0) {
                return KColorScheme().foreground(KColorScheme::NegativeText).color();
            } else if (index.column() == 1 && rootIndex == 0) {
                return KColorScheme().foreground(KColorScheme::PositiveText).color();
            }
        } else if (role == Role::GitItemType) {
            return (ItemType)rootIndex;
        }
    }

    return {};
}

QModelIndex GitStatusModel::indexForFilename(const QString &file)
{
    const auto ba = file.toUtf8();
    bool checkUntracked = m_nodes[Untrack].size() < 500;
    for (int i = 0; i < Untrack + int(checkUntracked); ++i) {
        const auto &items = m_nodes[i];
        int r = 0;
        for (const auto &item : items) {
            if (ba.endsWith(item.file)) {
                // match
                return index(r, 0, getModelIndex(static_cast<ItemType>(i)));
            }
            r++;
        }
    }
    return {};
}

void GitStatusModel::setStatusItems(GitUtils::GitParsedStatus status)
{
    beginResetModel();
    m_nodes[Staged] = std::move(status.staged);
    m_nodes[Changed] = std::move(status.changed);
    m_nodes[Conflict] = std::move(status.unmerge);
    m_nodes[Untrack] = std::move(status.untracked);
    m_nonUniqueFileNames = std::move(status.nonUniqueFileNames);
    endResetModel();
}
