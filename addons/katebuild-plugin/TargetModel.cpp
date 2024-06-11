/***************************************************************************
 *   This file is part of Kate build plugin                                *
 *   SPDX-FileCopyrightText: 2014 Kåre Särs <kare.sars@iki.fi>             *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "TargetModel.h"
#include <KLocalizedString>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTimer>
namespace
{
struct NodeInfo {
    int rootRow = -1;
    int targetSetRow = -1;
    int commandRow = -1;

    bool isCommand() const
    {
        return rootRow != -1 && targetSetRow != -1 && commandRow != -1;
    }

    bool isTargetSet() const
    {
        return rootRow != -1 && targetSetRow != -1 && commandRow == -1;
    }

    bool isRoot() const
    {
        return rootRow != -1 && targetSetRow == -1 && commandRow == -1;
    }

    bool isValid() const
    {
        return rootRow != -1;
    }
};
}

static QDebug operator<<(QDebug debug, const NodeInfo &node)
{
    QDebugStateSaver saver(debug);
    debug << "Node:" << node.rootRow << node.targetSetRow << node.commandRow;
    return debug;
}

TargetModel::TargetSet::TargetSet(const QString &_name, const QString &_dir, bool _loadedViaCMake, QString _cmakeConfigName)
    : name(_name)
    , workDir(_dir)
    , loadedViaCMake(_loadedViaCMake)
    , cmakeConfigName(_cmakeConfigName)
{
}

/**
 * This is the logical structure of the nodes in the model.
 * 1) In the root we have Session and/or Project
 * 2) Under these we have the TargetSets (working directory)
 * 3) Under the TargetSets we have the Targets (commands to execute and/or run commands)
 *
 * --- Session Targets
 *   - TargetSet
 *      - Command, Run
 *      - ...
 *   - TargetSet
 *      - Command, Run
 *      - ...
 *
 * --- Project Targets
 *   - TargetSet
 *      - Command, Run
 *      - ...
 *   - TargetSet
 *      - Command, Run
 *      - ...
 *
 *
 * How to interpret QModelIndex::internalId:
 * if internalId == InvalidIndex -> This is a Root element
 * else if internalId & TargetSetRowMask == TargetSetRowMask -> This is a TargetSet Node
 * else this is a command node
 *  (internalId == "parent rows")
 *
 * column 0 is command name, target-set name or Session/Project
 * column 1 is the command or working directory
 * column 2 is the run command
 */

// Topmost bit is used for RootRow 0 or 1
static constexpr int RootRowShift = sizeof(quintptr) * 8 - 1;

// One empty bit is reserved between RootRow and TargetSetRow
static constexpr quintptr TargetSetRowMask = ~((quintptr)3 << (RootRowShift - 1));

/** This function converts the internalId to root-row, which can be either 0 or 1 as we only can have two of them. '
 * @return 0 or 1 if successful or -1 if not a valid internalId
 */
static int idToRootRow(quintptr internalId)
{
    if (internalId == TargetModel::InvalidIndex) {
        return -1;
    }
    return internalId >> RootRowShift;
}

static int idToTargetSetRow(quintptr internalId)
{
    if (internalId == TargetModel::InvalidIndex) {
        return -1;
    }

    if ((internalId & TargetSetRowMask) == TargetSetRowMask) {
        return -1;
    }
    return internalId &= TargetSetRowMask;
}

static quintptr toInternalId(int rootRow, int targetSetRow)
{
    if (rootRow < 0) {
        return TargetModel::InvalidIndex;
    }

    if (targetSetRow < 0) {
        return TargetSetRowMask + ((quintptr)rootRow << RootRowShift);
    }

    return ((quintptr)targetSetRow & TargetSetRowMask) + ((quintptr)rootRow << RootRowShift);
}

static NodeInfo modelToNodeInfo(const QModelIndex &itemIndex)
{
    NodeInfo idx;
    if (!itemIndex.isValid()) {
        return idx;
    }

    if (itemIndex.internalId() == TargetModel::InvalidIndex) {
        // This is a root node
        idx.rootRow = itemIndex.row();
        return idx;
    }

    int rootRow = idToRootRow(itemIndex.internalId());
    int targetSetRow = idToTargetSetRow(itemIndex.internalId());
    if (rootRow != -1 && targetSetRow == -1) {
        // This is a TargetSet node
        idx.rootRow = rootRow;
        idx.targetSetRow = itemIndex.row();
    } //
    else if (rootRow != -1 && targetSetRow != -1) {
        // This is a Command node
        idx.rootRow = rootRow;
        idx.targetSetRow = targetSetRow;
        idx.commandRow = itemIndex.row();
    }
    return idx;
}

static bool nodeExists(const QList<TargetModel::RootNode> &rootNodes, const NodeInfo &node)
{
    if (!node.isValid()) {
        return false;
    }

    if (node.rootRow < 0 || node.rootRow >= rootNodes.size()) {
        return false;
    }

    if (node.isRoot()) {
        return true;
    }

    const QList<TargetModel::TargetSet> &targets = rootNodes[node.rootRow].targetSets;
    if (node.targetSetRow >= targets.size()) {
        return false;
    }

    if (node.isTargetSet()) {
        return true;
    }

    const QList<TargetModel::Command> &commands = targets[node.targetSetRow].commands;
    if (node.commandRow >= commands.size()) {
        qWarning() << "Command row out of bounds" << node;
        return false;
    }
    // The node is valid, not root and not a target-set and command-row is valid
    return true;
}

TargetModel::TargetModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_rootNodes.append(RootNode());
    m_rootNodes.append(RootNode());
    // By default the project branch is second
    m_rootNodes[1].isProject = true;
}
TargetModel::~TargetModel()
{
}

void TargetModel::clear(bool setSessionFirst)
{
    beginResetModel();
    m_rootNodes.clear();
    m_rootNodes.append(RootNode());
    m_rootNodes.append(RootNode());
    m_rootNodes[setSessionFirst ? 1 : 0].isProject = true;
    endResetModel();
}

QModelIndex TargetModel::sessionRootIndex() const
{
    for (int i = 0; i < m_rootNodes.size(); ++i) {
        if (!m_rootNodes[i].isProject) {
            return index(i, 0);
        }
    }
    return QModelIndex();
}

QModelIndex TargetModel::projectRootIndex() const
{
    for (int i = 0; i < m_rootNodes.size(); ++i) {
        if (m_rootNodes[i].isProject) {
            return index(i, 0);
        }
    }
    return QModelIndex();
}

QModelIndex TargetModel::insertTargetSetAfter(const QModelIndex &beforeIndex,
                                              const QString &setName,
                                              const QString &workDir,
                                              bool loadedViaCMake,
                                              const QString &cmakeConfig)
{
    // qDebug() << "Inserting TargetSet after:" << beforeIndex << setName <<workDir;
    NodeInfo bNode = modelToNodeInfo(beforeIndex);
    if (!nodeExists(m_rootNodes, bNode)) {
        // Add the new target-set to the end of the first root node (creating the root if needed)
        if (m_rootNodes.isEmpty()) {
            beginInsertRows(QModelIndex(), 0, 0);
            m_rootNodes.append(RootNode());
            endInsertRows();
        }
        bNode.rootRow = 0;
        bNode.targetSetRow = m_rootNodes[0].targetSets.size() - 1;
    }

    if (bNode.isRoot()) {
        bNode.targetSetRow = m_rootNodes[bNode.rootRow].targetSets.size() - 1;
    }

    QList<TargetSet> &targetSets = m_rootNodes[bNode.rootRow].targetSets;

    // Make the name unique
    QString newName = setName;
    for (int i = 0; i < targetSets.count(); i++) {
        if (targetSets[i].name == newName) {
            newName += QStringLiteral("+");
            i = -1;
        }
    }
    bNode.targetSetRow++;

    beginInsertRows(index(bNode.rootRow, 0), bNode.targetSetRow, bNode.targetSetRow);
    TargetModel::TargetSet targetSet(newName, workDir, loadedViaCMake, cmakeConfig);
    targetSets.insert(bNode.targetSetRow, targetSet);
    endInsertRows();
    if (m_rootNodes[bNode.rootRow].isProject) {
        Q_EMIT projectTargetChanged();
    }
    return index(bNode.targetSetRow, 0, index(bNode.rootRow, 0));
}

QModelIndex TargetModel::addCommandAfter(const QModelIndex &beforeIndex, const QString &cmdName, const QString &buildCmd, const QString &runCmd)
{
    // qDebug() << "addCommandAfter" << beforeIndex << cmdName;
    NodeInfo bNode = modelToNodeInfo(beforeIndex);
    if (!nodeExists(m_rootNodes, bNode)) {
        // Add the new command to the end of the first target-set of the first root node (creating the root and target-set if needed)
        if (m_rootNodes.isEmpty()) {
            beginInsertRows(QModelIndex(), 0, 0);
            m_rootNodes.append(RootNode());
            endInsertRows();
        }
        if (m_rootNodes[0].targetSets.isEmpty()) {
            beginInsertRows(index(0, 0), 0, 0);
            m_rootNodes[0].targetSets.append(TargetSet(i18n("Target Set"), QString(), false));
            endInsertRows();
        }
        bNode.rootRow = 0;
        bNode.targetSetRow = 0;
        bNode.commandRow = m_rootNodes[0].targetSets[0].commands.size() - 1;
    }

    if (bNode.isRoot()) {
        // Add the new command to the first target-set of this root node (creating the targetset if needed)
        if (m_rootNodes[bNode.rootRow].targetSets.isEmpty()) {
            beginInsertRows(index(bNode.rootRow, 0), 0, 0);
            m_rootNodes[bNode.rootRow].targetSets.append(TargetSet(i18n("Target Set"), QString(), false));
            endInsertRows();
        }
        bNode.targetSetRow = 0;
        bNode.commandRow = m_rootNodes[bNode.rootRow].targetSets[0].commands.size() - 1;
    }

    if (bNode.isTargetSet()) {
        bNode.commandRow = m_rootNodes[bNode.rootRow].targetSets[bNode.targetSetRow].commands.size() - 1;
    }

    // Now we have the place to insert the new command
    QList<Command> &commands = m_rootNodes[bNode.rootRow].targetSets[bNode.targetSetRow].commands;
    // make the name unique
    QString newName = cmdName;
    for (int i = 0; i < commands.count(); ++i) {
        if (commands[i].name == newName) {
            newName += QStringLiteral("+");
            i = -1;
        }
    }

    // it is the row after beforeIndex, where we want to insert the command
    bNode.commandRow++;

    QModelIndex targetSetIndex = index(bNode.targetSetRow, 0, index(bNode.rootRow, 0));
    beginInsertRows(targetSetIndex, bNode.commandRow, bNode.commandRow);
    commands.insert(bNode.commandRow, {.name = newName, .buildCmd = buildCmd, .runCmd = runCmd});
    endInsertRows();
    if (m_rootNodes[bNode.rootRow].isProject) {
        Q_EMIT projectTargetChanged();
    }
    return index(bNode.commandRow, 0, targetSetIndex);
}

QModelIndex TargetModel::cloneTargetOrSet(const QModelIndex &copyIndex)
{
    NodeInfo cpNode = modelToNodeInfo(copyIndex);
    if (!nodeExists(m_rootNodes, cpNode)) {
        return QModelIndex();
    }

    if (cpNode.isRoot()) {
        return QModelIndex();
    }

    QList<TargetSet> &targetSets = m_rootNodes[cpNode.rootRow].targetSets;
    QModelIndex rootIndex = index(cpNode.rootRow, 0);
    if (cpNode.isTargetSet()) {
        TargetSet targetSetCopy = targetSets[cpNode.targetSetRow];
        // Make the name unique
        QString newName = targetSetCopy.name;
        for (int i = 0; i < targetSets.size(); ++i) {
            if (targetSets[i].name == newName) {
                newName += QStringLiteral("+");
                i = -1;
            }
        }
        targetSetCopy.name = newName;
        beginInsertRows(rootIndex, cpNode.targetSetRow + 1, cpNode.targetSetRow + 1);
        targetSets.insert(cpNode.targetSetRow + 1, targetSetCopy);
        endInsertRows();
        if (m_rootNodes[cpNode.rootRow].isProject) {
            Q_EMIT projectTargetChanged();
        }
        return index(cpNode.targetSetRow + 1, 0, rootIndex);
    }

    // This is a command-row
    QList<Command> &commands = targetSets[cpNode.targetSetRow].commands;
    Command commandCopy = commands[cpNode.commandRow];
    QString newName = commandCopy.name;
    for (int i = 0; i < commands.size(); i++) {
        if (commands[i].name == newName) {
            newName += QStringLiteral("+");
            i = -1;
        }
    }
    commandCopy.name = newName;
    QModelIndex tgSetIndex = index(cpNode.targetSetRow, 0, rootIndex);
    beginInsertRows(tgSetIndex, cpNode.commandRow + 1, cpNode.commandRow + 1);
    commands.insert(cpNode.commandRow + 1, commandCopy);
    endInsertRows();
    if (m_rootNodes[cpNode.rootRow].isProject) {
        Q_EMIT projectTargetChanged();
    }
    return index(cpNode.commandRow + 1, 0, tgSetIndex);
}

void TargetModel::deleteItem(const QModelIndex &itemIndex)
{
    if (!itemIndex.isValid()) {
        return;
    }

    NodeInfo node = modelToNodeInfo(itemIndex);
    if (!nodeExists(m_rootNodes, node)) {
        qDebug() << "Node does not exist:" << node;
        return;
    }

    bool wasProjectNode = m_rootNodes[node.rootRow].isProject;

    if (node.isRoot()) {
        beginRemoveRows(itemIndex, 0, m_rootNodes[node.rootRow].targetSets.size() - 1);
        m_rootNodes[node.rootRow].targetSets.clear();
        endRemoveRows();
    } else if (node.isTargetSet()) {
        beginRemoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row());
        m_rootNodes[node.rootRow].targetSets.removeAt(node.targetSetRow);
        endRemoveRows();
    } else {
        beginRemoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row());
        m_rootNodes[node.rootRow].targetSets[node.targetSetRow].commands.removeAt(node.commandRow);
        endRemoveRows();
    }
    if (wasProjectNode) {
        Q_EMIT projectTargetChanged();
    }
}

void TargetModel::deleteProjectTargerts()
{
    for (int i = 0; i < m_rootNodes.count(); ++i) {
        if (m_rootNodes[i].isProject && m_rootNodes[i].targetSets.count() > 0) {
            beginRemoveRows(index(i, 0), 0, m_rootNodes[i].targetSets.count() - 1);
            m_rootNodes[i].targetSets.clear();
            endRemoveRows();
            return;
        }
    }
}

void TargetModel::moveRowUp(const QModelIndex &itemIndex)
{
    if (!itemIndex.isValid()) {
        return;
    }
    NodeInfo node = modelToNodeInfo(itemIndex);
    if (!nodeExists(m_rootNodes, node)) {
        qDebug() << "Node does not exist:" << node;
        return;
    }
    int row = itemIndex.row();
    if (row == 0) {
        return; // This is valid for all the three cases
    }

    QModelIndex parent = itemIndex.parent(); // This parent is valid for all the cases

    if (node.isRoot()) {
        beginMoveRows(parent, row, row, parent, row - 1);
        m_rootNodes.move(row, row - 1);
        endMoveRows();
        return;
    }

    QList<TargetSet> &targetSets = m_rootNodes[node.rootRow].targetSets;
    if (node.isTargetSet()) {
        beginMoveRows(parent, row, row, parent, row - 1);
        targetSets.move(row, row - 1);
        endMoveRows();
        if (m_rootNodes[node.rootRow].isProject) {
            Q_EMIT projectTargetChanged();
        }
        return;
    }

    // It is a command-row
    QList<Command> &commands = targetSets[node.targetSetRow].commands;
    beginMoveRows(parent, row, row, parent, row - 1);
    commands.move(row, row - 1);
    endMoveRows();
    if (m_rootNodes[node.rootRow].isProject) {
        Q_EMIT projectTargetChanged();
    }
}

void TargetModel::moveRowDown(const QModelIndex &itemIndex)
{
    if (!itemIndex.isValid()) {
        return;
    }
    NodeInfo node = modelToNodeInfo(itemIndex);
    if (!nodeExists(m_rootNodes, node)) {
        qDebug() << "Node does not exist:" << node;
        return;
    }

    // These are valid for all the row types
    int row = itemIndex.row();
    QModelIndex parent = itemIndex.parent();

    if (node.isRoot()) {
        if (row >= m_rootNodes.size() - 1) {
            return;
        }
        beginMoveRows(parent, row, row, parent, row + 2);
        m_rootNodes.move(row, row + 1);
        endMoveRows();
        return;
    }

    QList<TargetSet> &targetSets = m_rootNodes[node.rootRow].targetSets;
    if (node.isTargetSet()) {
        beginMoveRows(parent, row, row, parent, row + 2);
        targetSets.move(row, row + 1);
        endMoveRows();
        if (m_rootNodes[node.rootRow].isProject) {
            Q_EMIT projectTargetChanged();
        }
        return;
    }

    // It is a command-row
    QList<Command> &commands = targetSets[node.targetSetRow].commands;
    beginMoveRows(parent, row, row, parent, row + 2);
    commands.move(row, row + 1);
    endMoveRows();
    if (m_rootNodes[node.rootRow].isProject) {
        Q_EMIT projectTargetChanged();
    }
}

const QList<TargetModel::TargetSet> TargetModel::sessionTargetSets() const
{
    for (int i = 0; i < m_rootNodes.size(); ++i) {
        if (m_rootNodes[i].isProject == false) {
            return m_rootNodes[i].targetSets;
        }
    }
    return QList<TargetModel::TargetSet>();
}

QVariant TargetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qWarning() << "Invalid index" << index;
        return QVariant();
    }

    NodeInfo node = modelToNodeInfo(index);
    if (!nodeExists(m_rootNodes, node)) {
        qDebug() << "Node does not exist:" << node;
        return QVariant();
    }

    if (node.isRoot()) {
        if ((role == Qt::DisplayRole || role == Qt::ToolTipRole) && index.column() == 0) {
            return m_rootNodes[node.rootRow].isProject ? i18n("Project") : i18n("Session");
        } else if (role == RowTypeRole) {
            return RowType::RootRow;
        } else if (role == IsProjectTargetRole) {
            return m_rootNodes[node.rootRow].isProject;
        }
        return QVariant();
    }

    // This is either a TargetSet or a Command
    const TargetSet &targetSet = m_rootNodes[node.rootRow].targetSets[node.targetSetRow];

    if (node.isTargetSet()) {
        // This is a TargetSet node
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            switch (index.column()) {
            case 0:
                return targetSet.name;
            case 1:
                return targetSet.workDir;
            }
            break;
        case CommandRole:
            if (targetSet.commands.isEmpty()) {
                return QVariant();
            }
            return targetSet.commands[0].buildCmd;
        case CommandNameRole:
            if (targetSet.commands.isEmpty()) {
                return QVariant();
            }
            return targetSet.commands[0].name;
        case WorkDirRole:
            return targetSet.workDir.isEmpty() ? QString() : targetSet.workDir.split(QLatin1Char(';')).first();
        case SearchPathsRole:
            return targetSet.workDir.split(QLatin1Char(';'));
        case TargetSetNameRole:
            return targetSet.name;
        case RowTypeRole:
            return TargetSetRow;
        case IsProjectTargetRole:
            return m_rootNodes[node.rootRow].isProject;
        }
    }

    if (node.isCommand()) {
        const Command &command = targetSet.commands[node.commandRow];
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            switch (index.column()) {
            case 0:
                return command.name;
            case 1:
                return command.buildCmd;
            case 2:
                return command.runCmd;
            }
            break;
        case CommandRole:
            return command.buildCmd;
        case CommandNameRole:
            return command.name;
        case WorkDirRole:
            return targetSet.workDir.isEmpty() ? QString() : targetSet.workDir.split(QLatin1Char(';')).first();
        case SearchPathsRole:
            return targetSet.workDir.split(QLatin1Char(';'));
        case TargetSetNameRole:
            return targetSet.name;
        case RowTypeRole:
            return CommandRow;
        case IsProjectTargetRole:
            return m_rootNodes[node.rootRow].isProject;
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
    if (!itemIndex.isValid()) {
        qWarning() << "Invalid index";
        return false;
    }

    NodeInfo node = modelToNodeInfo(itemIndex);
    if (!nodeExists(m_rootNodes, node)) {
        qDebug() << "Node does not exist:" << node;
        return false;
    }

    if (node.isRoot()) {
        return false;
    }

    // This is either a TargetSet or a Command
    TargetSet &targetSet = m_rootNodes[node.rootRow].targetSets[node.targetSetRow];

    bool editDone = false;
    if (node.isTargetSet()) {
        switch (itemIndex.column()) {
        case 0:
            targetSet.name = value.toString();
            editDone = true;
            break;
        case 1:
            targetSet.workDir = value.toString();
            editDone = true;
            break;
        }
    } else {
        switch (itemIndex.column()) {
        case 0:
            targetSet.commands[node.commandRow].name = value.toString();
            editDone = true;
            break;
        case 1:
            targetSet.commands[node.commandRow].buildCmd = value.toString();
            editDone = true;
            break;
        case 2:
            targetSet.commands[node.commandRow].runCmd = value.toString();
            editDone = true;
            break;
        }
    }
    if (editDone) {
        Q_EMIT dataChanged(itemIndex, itemIndex);
        if (m_rootNodes[node.rootRow].isProject) {
            Q_EMIT projectTargetChanged();
        }
        return true;
    }
    return false;
}

Qt::ItemFlags TargetModel::flags(const QModelIndex &itemIndex) const
{
    if (!itemIndex.isValid()) {
        return Qt::NoItemFlags;
    }

    NodeInfo node = modelToNodeInfo(itemIndex);
    if (!nodeExists(m_rootNodes, node)) {
        return Qt::NoItemFlags;
    }
    if (node.isRoot()) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    // run command column for target set row
    if (itemIndex.column() == 2 && node.isTargetSet()) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int TargetModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        // Invalid index -> root
        return m_rootNodes.size();
    }

    NodeInfo node = modelToNodeInfo(parent);
    if (!nodeExists(m_rootNodes, node)) {
        // uncomment for debugging
        // qDebug() << "Node does not exist:" << node << parent;
        return 0;
    }

    if (parent.column() != 0) {
        // Only first column has children
        return 0;
    }

    if (node.isRoot()) {
        return m_rootNodes[node.rootRow].targetSets.size();
    }

    if (node.isTargetSet()) {
        return m_rootNodes[node.rootRow].targetSets[node.targetSetRow].commands.size();
    }

    // This is a command node -> no children
    return 0;
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

    if (!parent.isValid()) {
        // RootRow Item (Session/Project)
        if (row >= m_rootNodes.size()) {
            return QModelIndex();
        }
        return createIndex(row, column, InvalidIndex);
    }

    if (parent.column() != 0) {
        // Only column 0 can have children.
        return QModelIndex();
    }

    if (parent.internalId() == InvalidIndex) {
        // TargetSet node
        int rootRow = parent.row();
        if (rootRow >= m_rootNodes.size() || row >= m_rootNodes.at(rootRow).targetSets.size()) {
            return QModelIndex();
        }
        return createIndex(row, column, toInternalId(rootRow, -1));
    }

    // This is a command node
    int rootRow = idToRootRow(parent.internalId());
    int targetSetRow = parent.row();
    if (rootRow >= m_rootNodes.size() || targetSetRow >= m_rootNodes.at(rootRow).targetSets.size()) {
        return QModelIndex();
    }
    const TargetSet &tgSet = m_rootNodes.at(rootRow).targetSets.at(targetSetRow);
    if (row >= tgSet.commands.size()) {
        return QModelIndex();
    }
    return createIndex(row, column, toInternalId(rootRow, targetSetRow));
}

QModelIndex TargetModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    if (child.internalId() == InvalidIndex) {
        // child is a RootRow node -> invalid parent
        return QModelIndex();
    }

    int rootRow = idToRootRow(child.internalId());
    int targetSetRow = idToTargetSetRow(child.internalId());

    if (targetSetRow == -1) {
        // child is a TargetSetNode
        return createIndex(rootRow, 0, InvalidIndex);
    }

    // child is a command node
    return createIndex(targetSetRow, 0, toInternalId(rootRow, -1));
}

static QJsonObject toJson(const TargetModel::Command &target)
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = target.name;
    obj[QStringLiteral("build_cmd")] = target.buildCmd;
    obj[QStringLiteral("run_cmd")] = target.runCmd;
    return obj;
}

static QJsonObject toJson(const TargetModel::TargetSet &set)
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = set.name;
    obj[QStringLiteral("directory")] = set.workDir;

    QJsonArray targets;
    for (const auto &target : set.commands) {
        targets << toJson(target);
    }
    obj[QStringLiteral("targets")] = targets;
    return obj;
}

static QJsonObject toJson(const TargetModel::RootNode &root)
{
    QJsonObject obj;
    QJsonArray sets;
    for (const auto &set : root.targetSets) {
        sets << toJson(set);
    }
    obj[QStringLiteral("target_sets")] = sets;
    return obj;
}

bool TargetModel::validTargetsJson(const QString &jsonStr) const
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        return false;
    }
    const QJsonObject obj = doc.object();
    return obj.contains(QStringLiteral("target_sets")) || obj.contains(QStringLiteral("targets")) || obj.contains(QStringLiteral("name"));
}

QJsonObject TargetModel::indexToJsonObj(const QModelIndex &modelIndex) const
{
    NodeInfo node = modelToNodeInfo(modelIndex);
    if (!nodeExists(m_rootNodes, node)) {
        return QJsonObject();
    }

    QJsonObject obj;
    if (node.isRoot()) {
        obj = toJson(m_rootNodes[node.rootRow]);
    } else if (node.isTargetSet()) {
        obj = toJson(m_rootNodes[node.rootRow].targetSets[node.targetSetRow]);
    } else if (node.isCommand()) {
        obj = toJson(m_rootNodes[node.rootRow].targetSets[node.targetSetRow].commands[node.commandRow]);
    }

    return obj;
}

QString TargetModel::indexToJson(const QModelIndex &modelIndex) const
{
    QJsonDocument doc(indexToJsonObj(modelIndex));
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QModelIndex TargetModel::insertAfter(const QModelIndex &modelIndex, const QString &jsonStr)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Could not parse the provided Json";
        return QModelIndex();
    }
    return insertAfter(modelIndex, doc.object());
}

QModelIndex TargetModel::insertAfter(const QModelIndex &modelIndex, const QJsonObject &obj)
{
    QModelIndex currentIndex = modelIndex;
    if (obj.contains(QStringLiteral("target_sets"))) {
        const QJsonArray sets = obj[QStringLiteral("target_sets")].toArray();
        for (const auto &set : sets) {
            currentIndex = insertAfter(currentIndex, set.toObject());
            if (!currentIndex.isValid()) {
                qWarning() << "Failed to insert targetset";
                return QModelIndex();
            }
        }
    } else if (obj.contains(QStringLiteral("targets"))) {
        QString dir = obj[QStringLiteral("directory")].toString();
        QString name = obj[QStringLiteral("name")].toString();
        currentIndex = insertTargetSetAfter(currentIndex, name, dir);
        QModelIndex setIndex = currentIndex;
        const QJsonArray targets = obj[QStringLiteral("targets")].toArray();
        for (const auto target : targets) {
            currentIndex = insertAfter(currentIndex, target.toObject());
            if (!currentIndex.isValid()) {
                qWarning() << "Failed to insert target";
                break;
            }
        }
        currentIndex = setIndex;
    } else if (obj.contains(QStringLiteral("name"))) {
        QString name = obj[QStringLiteral("name")].toString();
        QString cmd = obj[QStringLiteral("build_cmd")].toString();
        QString run = obj[QStringLiteral("run_cmd")].toString();
        currentIndex = addCommandAfter(currentIndex, name, cmd, run);
    }

    return currentIndex;
}

#include "moc_TargetModel.cpp"
