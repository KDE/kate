/***************************************************************************
 *   This file is part of Kate build plugin                                *
 *   SPDX-FileCopyrightText: 2014 Kåre Särs <kare.sars@iki.fi>             *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "TargetModel.h"
#include <KLocalizedString>
#include <QDebug>
#include <QTimer>

TargetModel::TargetSet::TargetSet(const QString &_name, const QString &_dir)
    : name(_name)
    , workDir(_dir)
{
}

// Model data
// parent is the m_targets list index or InvalidIndex if it is a root element
// row is the m_targets.commands list index or m_targets index if it is a root element
// column 0 is command name or target-set name if it is a root element
// column 1 is the command or working directory if it is a root element

TargetModel::TargetModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}
TargetModel::~TargetModel()
{
}

void TargetModel::clear()
{
    beginResetModel();
    m_targets.clear();
    endResetModel();
}

QModelIndex TargetModel::addTargetSet(const QString &setName, const QString &workDir)
{
    return insertTargetSet(m_targets.count(), setName, workDir);
}

QModelIndex TargetModel::insertTargetSet(int row, const QString &setName, const QString &workDir)
{
    if (row < 0 || row > m_targets.count()) {
        qWarning() << "Row index out of bounds:" << row << m_targets.count();
    }

    // make the name unique
    QString newName = setName;
    for (int i = 0; i < m_targets.count(); i++) {
        if (m_targets[i].name == newName) {
            newName += QStringLiteral("+");
            i = -1;
        }
    }

    beginInsertRows(QModelIndex(), row, row);
    TargetModel::TargetSet targetSet(newName, workDir);
    m_targets.insert(row, targetSet);
    endInsertRows();
    return index(row, 0);
}

QModelIndex TargetModel::addCommand(const QModelIndex &parentIndex, const QString &cmdName, const QString &buildCmd, const QString &runCmd)
{
    int rootRow = parentIndex.row();
    if (rootRow < 0 || rootRow >= m_targets.size()) {
        qDebug() << "rootRow" << rootRow << "not valid" << m_targets.size();
        return QModelIndex();
    }

    // make the name unique
    QString newName = cmdName;
    for (int i = 0; i < m_targets[rootRow].commands.count(); i++) {
        if (m_targets[rootRow].commands[i].name == newName) {
            newName += QStringLiteral("+");
            i = -1;
        }
    }

    QModelIndex rootIndex = createIndex(rootRow, 0, InvalidIndex);
    beginInsertRows(rootIndex, m_targets[rootRow].commands.count(), m_targets[rootRow].commands.count());
    m_targets[rootRow].commands << Command{newName, buildCmd, runCmd};
    endInsertRows();
    return createIndex(m_targets[rootRow].commands.size() - 1, 0, rootRow);
}

QModelIndex TargetModel::copyTargetOrSet(const QModelIndex &index)
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    quint32 rootRow = index.internalId();
    if (rootRow == InvalidIndex) {
        rootRow = index.row();
        if (m_targets.count() <= static_cast<int>(rootRow)) {
            return QModelIndex();
        }

        beginInsertRows(QModelIndex(), rootRow + 1, rootRow + 1);
        QString newName = m_targets[rootRow].name + QStringLiteral("+");
        for (int i = 0; i < m_targets.count(); i++) {
            if (m_targets[i].name == newName) {
                newName += QStringLiteral("+");
                i = -1;
            }
        }
        m_targets.insert(rootRow + 1, m_targets[rootRow]);
        m_targets[rootRow + 1].name = newName;
        endInsertRows();

        return createIndex(rootRow + 1, 0, InvalidIndex);
    }

    if (m_targets.count() <= static_cast<int>(rootRow)) {
        return QModelIndex();
    }
    int cmdRow = index.row();
    if (cmdRow < 0) {
        return QModelIndex();
    }
    if (cmdRow >= m_targets[rootRow].commands.count()) {
        return QModelIndex();
    }

    QModelIndex rootIndex = createIndex(rootRow, 0, InvalidIndex);
    beginInsertRows(rootIndex, cmdRow + 1, cmdRow + 1);
    const auto cmd = m_targets[rootRow].commands[cmdRow];
    QString newName = cmd.name + QStringLiteral("+");
    for (int i = 0; i < m_targets[rootRow].commands.count(); i++) {
        if (m_targets[rootRow].commands[i].name == newName) {
            newName += QStringLiteral("+");
            i = -1;
        }
    }
    m_targets[rootRow].commands.insert(cmdRow + 1, Command{newName, cmd.buildCmd, cmd.runCmd});
    endInsertRows();
    return createIndex(cmdRow + 1, 0, rootRow);
}

void TargetModel::deleteItem(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    if (index.internalId() == InvalidIndex) {
        if (index.row() < 0 || index.row() >= m_targets.size()) {
            qWarning() << "Bad target-set row:" << index.row() << m_targets.size();
            return;
        }
        beginRemoveRows(index.parent(), index.row(), index.row());
        m_targets.removeAt(index.row());
        endRemoveRows();
    } else {
        int setRow = static_cast<int>(index.internalId());
        if (setRow >= m_targets.size()) {
            qWarning() << "Bad target-set row:" << index.internalId() << m_targets.size();
            return;
        }
        TargetSet &set = m_targets[setRow];
        if (index.row() < 0 || index.row() >= set.commands.size()) {
            qWarning() << "Bad command row:" << index.row() << set.commands.size();
            return;
        }
        beginRemoveRows(index.parent(), index.row(), index.row());
        set.commands.removeAt(index.row());
        endRemoveRows();
    }
}

void TargetModel::deleteTargetSet(const QString &targetSet)
{
    for (int i = 0; i < m_targets.count(); i++) {
        if (m_targets[i].name == targetSet) {
            beginRemoveRows(QModelIndex(), i, i);
            m_targets.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}

void TargetModel::moveRowUp(const QModelIndex &itemIndex)
{
    if (!itemIndex.isValid() || !hasIndex(itemIndex.row(), itemIndex.column(), itemIndex.parent())) {
        return;
    }

    QModelIndex parent = itemIndex.parent();
    int row = itemIndex.row();
    if (row < 1) {
        return;
    }
    beginMoveRows(parent, row, row, parent, row - 1);
    if (!parent.isValid()) {
        m_targets.move(row, row - 1);
    } else {
        int rootRow = itemIndex.internalId();
        if (rootRow < 0 || rootRow >= m_targets.size()) {
            qWarning() << "Bad root row index" << rootRow << m_targets.size();
            return;
        }
        m_targets[rootRow].commands.move(row, row - 1);
    }
    endMoveRows();
}

void TargetModel::moveRowDown(const QModelIndex &itemIndex)
{
    if (!itemIndex.isValid() || !hasIndex(itemIndex.row(), itemIndex.column(), itemIndex.parent())) {
        return;
    }

    QModelIndex parent = itemIndex.parent();
    int row = itemIndex.row();
    if (!parent.isValid()) {
        if (!parent.isValid() && row >= m_targets.size() - 1) {
            return;
        }
        beginMoveRows(parent, row, row, parent, row + 2);
        m_targets.move(row, row + 1);
        endMoveRows();
    } else {
        int rootRow = itemIndex.internalId();
        if (rootRow < 0 || rootRow >= m_targets.size()) {
            qWarning() << "Bad root row index" << rootRow << m_targets.size();
            return;
        }
        if (row >= m_targets[rootRow].commands.size() - 1) {
            return;
        }
        beginMoveRows(parent, row, row, parent, row + 2);
        m_targets[rootRow].commands.move(row, row + 1);
        endMoveRows();
    }
}

const QString TargetModel::command(const QModelIndex &itemIndex)
{
    if (!itemIndex.isValid()) {
        return QString();
    }

    // take the item from the second column
    QModelIndex cmdIndex = itemIndex.siblingAtColumn(1);

    if (!itemIndex.parent().isValid()) {
        // This is the target-set root column
        // execute the default target (checked or first child)
        auto model = itemIndex.model();
        if (!model) {
            qDebug() << "No model found";
            return QString();
        }
        for (int i = 0; i < model->rowCount(itemIndex); ++i) {
            QModelIndex childIndex = model->index(i, 0, itemIndex);
            if (i == 0) {
                cmdIndex = childIndex.siblingAtColumn(1);
            }
        }
    }
    return cmdIndex.data().toString();
}

const QString TargetModel::cmdName(const QModelIndex &itemIndex)
{
    if (!itemIndex.isValid()) {
        return QString();
    }

    // take the item from the first column
    QModelIndex nameIndex = itemIndex.sibling(itemIndex.row(), 0);

    if (!itemIndex.parent().isValid()) {
        // This is the target-set root column
        // execute the default target (checked or first child)
        auto model = itemIndex.model();
        if (!model) {
            qDebug() << "No model found";
            return QString();
        }
        for (int i = 0; i < model->rowCount(itemIndex); ++i) {
            QModelIndex childIndex = model->index(i, 0, itemIndex);
            if (i == 0) {
                nameIndex = childIndex.siblingAtColumn(0);
            }
        }
    }
    return nameIndex.data().toString();
}

const QString TargetModel::workDir(const QModelIndex &itemIndex)
{
    QStringList paths = searchPaths(itemIndex);
    return paths.isEmpty() ? QString() : paths.first();
}

const QStringList TargetModel::searchPaths(const QModelIndex &itemIndex)
{
    if (!itemIndex.isValid()) {
        return QStringList();
    }

    QModelIndex workDirIndex = itemIndex.sibling(itemIndex.row(), 1);

    if (itemIndex.parent().isValid()) {
        workDirIndex = itemIndex.parent().siblingAtColumn(1);
    }
    return workDirIndex.data().toString().split(QLatin1Char(';'));
}

const QString TargetModel::targetName(const QModelIndex &itemIndex)
{
    if (!itemIndex.isValid()) {
        return QString();
    }
    QModelIndex targetNameIndex = itemIndex.sibling(itemIndex.row(), 0);

    if (itemIndex.parent().isValid()) {
        targetNameIndex = itemIndex.parent().siblingAtColumn(0);
    }
    return targetNameIndex.data().toString();
}

QVariant TargetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !hasIndex(index.row(), index.column(), index.parent())) {
        return QVariant();
    }

    // Tooltip
    if (role == Qt::ToolTipRole) {
        if (index.column() == 0 && index.parent().isValid()) {
            return i18n("Check the check-box to make the command the default for the target-set.");
        }
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole) {
        return QVariant();
    }

    int row = index.row();

    if (index.internalId() == InvalidIndex) {
        if (row < 0 || row >= m_targets.size()) {
            return QVariant();
        }

        switch (index.column()) {
        case 0:
            return m_targets[row].name;
        case 1:
            return m_targets[row].workDir;
        }
    } else {
        int rootIndex = index.internalId();
        if (rootIndex < 0 || rootIndex >= m_targets.size()) {
            return QVariant();
        }
        if (row < 0 || row >= m_targets[rootIndex].commands.size()) {
            return QVariant();
        }

        if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
            switch (index.column()) {
            case 0:
                return m_targets[rootIndex].commands[row].name;
            case 1:
                return m_targets[rootIndex].commands[row].buildCmd;
            case 2:
                return m_targets[rootIndex].commands[row].runCmd;
            }
        }
    }

    return QVariant();
}

QVariant TargetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    if (section == 0) {
        return i18n("Command/Target-set Name");
    }
    if (section == 1) {
        return i18n("Working Directory / Command");
    }
    if (section == 2) {
        return i18n("Run Command");
    }
    return QVariant();
}

bool TargetModel::setData(const QModelIndex &itemIndex, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }
    if (!itemIndex.isValid() || !hasIndex(itemIndex.row(), itemIndex.column(), itemIndex.parent())) {
        return false;
    }

    int row = itemIndex.row();
    if (itemIndex.internalId() == InvalidIndex) {
        if (row < 0 || row >= m_targets.size()) {
            return false;
        }
        switch (itemIndex.column()) {
        case 0:
            m_targets[row].name = value.toString();
            Q_EMIT dataChanged(createIndex(row, 0), createIndex(row, 0));
            return true;
        case 1:
            Q_EMIT dataChanged(createIndex(row, 1), createIndex(row, 1));
            m_targets[row].workDir = value.toString();
            return true;
        }
    } else {
        int rootRow = itemIndex.internalId();
        if (rootRow < 0 || rootRow >= m_targets.size()) {
            return false;
        }
        if (row < 0 || row >= m_targets[rootRow].commands.size()) {
            return false;
        }

        QModelIndex rootIndex = createIndex(rootRow, 0);
        switch (itemIndex.column()) {
        case 0:
            m_targets[rootRow].commands[row].name = value.toString();
            Q_EMIT dataChanged(index(row, 0, rootIndex), index(row, 0, rootIndex));
            return true;
        case 1:
            m_targets[rootRow].commands[row].buildCmd = value.toString();
            Q_EMIT dataChanged(index(row, 1, rootIndex), index(row, 1, rootIndex));
            return true;
        case 2:
            m_targets[rootRow].commands[row].runCmd = value.toString();
            Q_EMIT dataChanged(index(row, 2, rootIndex), index(row, 2, rootIndex));
            return true;
        }
    }
    return false;
}

Qt::ItemFlags TargetModel::flags(const QModelIndex &itemIndex) const
{
    if (!itemIndex.isValid()) {
        return Qt::NoItemFlags;
    }

    // run command column for target set row
    if (itemIndex.column() == 2 && !itemIndex.parent().isValid()) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int TargetModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_targets.size();
    }

    if (parent.internalId() != InvalidIndex) {
        return 0;
    }

    if (parent.column() != 0) {
        return 0;
    }

    int row = parent.row();
    if (row < 0 || row >= m_targets.size()) {
        return 0;
    }

    return m_targets[row].commands.size();
}

int TargetModel::columnCount(const QModelIndex &) const
{
    return 3;
}

QModelIndex TargetModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0) {
        return QModelIndex();
    }

    quint32 rootRow = InvalidIndex;
    if (parent.isValid() && parent.internalId() == InvalidIndex) {
        // This is a command (child of a root element)
        if (parent.column() != 0) {
            // Only root item column 0 can have children
            return QModelIndex();
        }
        rootRow = parent.row();
        if (parent.row() >= m_targets.size() || row >= m_targets.at(parent.row()).commands.size()) {
            return QModelIndex();
        }
        return createIndex(row, column, rootRow);
    }
    // This is a root item
    if (row >= m_targets.size()) {
        return QModelIndex();
    }
    return createIndex(row, column, rootRow);
}

QModelIndex TargetModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    if (child.internalId() == InvalidIndex) {
        return QModelIndex();
    }

    int pRow = (int)child.internalId();

    if (pRow < 0 || pRow >= m_targets.size()) {
        return QModelIndex();
    }
    return createIndex(pRow, 0, InvalidIndex);
}
