/***************************************************************************
 *   This file is part of Kate build plugin                                *
 *   SPDX-FileCopyrightText: 2014 Kåre Särs <kare.sars@iki.fi>             *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "TargetModel.h"
#include <KLocalizedString>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTimer>

using namespace Qt::Literals::StringLiterals;

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

TargetModel::TargetSet::TargetSet(const QString &_name, const QString &_dir, bool _loadedViaCMake, const QString &_cmakeConfigName)
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
static constexpr int RootRowShift = (sizeof(quintptr) * 8) - 1;

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

TargetModel::~TargetModel() = default;

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
    return {};
}

QModelIndex TargetModel::projectRootIndex() const
{
    for (int i = 0; i < m_rootNodes.size(); ++i) {
        if (m_rootNodes[i].isProject) {
            return index(i, 0);
        }
    }
    return {};
}

QModelIndex TargetModel::insertTargetSetAfter(const QModelIndex &beforeIndex,
                                              const QString &setName,
                                              const QString &workDir,
                                              bool loadedViaCMake,
                                              const QString &cmakeConfig,
                                              const QString &projectBaseDir)
{
    // qDebug() << "Inserting TargetSet after:" << beforeIndex << setName << workDir
    // << cmakeConfig << projectBaseDir;
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

    if (loadedViaCMake) {
        // loadedViaCMake target-sets replace the previous with the same name
        for (int i = 0; i < targetSets.count(); i++) {
            if (targetSets[i].name == setName) {
                beginRemoveRows(index(bNode.rootRow, 0), i, i);
                m_rootNodes[bNode.rootRow].targetSets.removeAt(i);
                endRemoveRows();
                bNode.targetSetRow = i - 1;
            }
        }
    }

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
    targetSet.projectBaseDir = projectBaseDir;
    targetSets.insert(bNode.targetSetRow, targetSet);
    endInsertRows();
    if (m_rootNodes[bNode.rootRow].isProject) {
        Q_EMIT projectTargetChanged(targetSet.projectBaseDir);
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
            m_rootNodes[0].targetSets.append(TargetSet(i18n("Target Set"), QDir::homePath(), false));
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
            m_rootNodes[bNode.rootRow].targetSets.append(TargetSet(i18n("Target Set"), QDir::homePath(), false));
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
        Q_EMIT projectTargetChanged(m_rootNodes[bNode.rootRow].targetSets[bNode.targetSetRow].projectBaseDir);
    }
    return index(bNode.commandRow, 0, targetSetIndex);
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
    QString projectBaseDir;

    if (node.isRoot()) {
        beginRemoveRows(itemIndex, 0, m_rootNodes[node.rootRow].targetSets.size() - 1);
        m_rootNodes[node.rootRow].targetSets.clear();
        endRemoveRows();
    } else if (node.isTargetSet()) {
        beginRemoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row());
        projectBaseDir = m_rootNodes[node.rootRow].targetSets[node.targetSetRow].projectBaseDir;
        m_rootNodes[node.rootRow].targetSets.removeAt(node.targetSetRow);
        endRemoveRows();
    } else {
        beginRemoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row());
        projectBaseDir = m_rootNodes[node.rootRow].targetSets[node.targetSetRow].projectBaseDir;
        m_rootNodes[node.rootRow].targetSets[node.targetSetRow].commands.removeAt(node.commandRow);
        endRemoveRows();
    }
    if (wasProjectNode) {
        Q_EMIT projectTargetChanged(projectBaseDir);
    }
}

void TargetModel::deleteProjectTargetsExcept(const QList<QString> &keep)
{
    for (int i = 0; i < m_rootNodes.count(); ++i) {
        if (m_rootNodes[i].isProject) {
            auto projs = m_rootNodes[i];
            for (int j = m_rootNodes[i].targetSets.count() - 1; j >= 0; --j) {
                if (keep.contains(m_rootNodes[i].targetSets[j].projectBaseDir)) {
                    continue;
                }
                beginRemoveRows(index(i, 0), j, j);
                m_rootNodes[i].targetSets.removeAt(j);
                endRemoveRows();
            }
            return;
        }
    }
}

void TargetModel::deleteProjectTargets(const QString &baseDir)
{
    for (int i = 0; i < m_rootNodes.count(); ++i) {
        if (m_rootNodes[i].isProject) {
            auto projs = m_rootNodes[i];
            for (int j = m_rootNodes[i].targetSets.count() - 1; j >= 0; --j) {
                if (baseDir == m_rootNodes[i].targetSets[j].projectBaseDir) {
                    beginRemoveRows(index(i, 0), j, j);
                    m_rootNodes[i].targetSets.removeAt(j);
                    endRemoveRows();
                    return;
                }
            }
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
        if (beginMoveRows(parent, row, row, parent, row - 1)) {
            m_rootNodes.move(row, row - 1);
            endMoveRows();
        } else {
            Q_ASSERT(false);
        }
        return;
    }

    QList<TargetSet> &targetSets = m_rootNodes[node.rootRow].targetSets;
    if (node.isTargetSet()) {
        auto dir = m_rootNodes[node.rootRow].targetSets[row].projectBaseDir;
        if (beginMoveRows(parent, row, row, parent, row - 1)) {
            targetSets.move(row, row - 1);
            endMoveRows();
            if (m_rootNodes[node.rootRow].isProject) {
                Q_EMIT projectTargetChanged(dir);
            }
        } else {
            Q_ASSERT(false);
        }
        return;
    }

    // It is a command-row
    if (beginMoveRows(parent, row, row, parent, row - 1)) {
        QList<Command> &commands = targetSets[node.targetSetRow].commands;
        commands.move(row, row - 1);
        endMoveRows();
        if (m_rootNodes[node.rootRow].isProject) {
            Q_EMIT projectTargetChanged(targetSets[node.targetSetRow].projectBaseDir);
        }
    } else {
        Q_ASSERT(false);
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
        if (beginMoveRows(parent, row, row, parent, row + 2)) {
            m_rootNodes.move(row, row + 1);
            endMoveRows();
        } else {
            Q_ASSERT(false);
        }
        return;
    }

    QList<TargetSet> &targetSets = m_rootNodes[node.rootRow].targetSets;
    if (node.isTargetSet()) {
        QString dir = targetSets[row].projectBaseDir;
        if (beginMoveRows(parent, row, row, parent, row + 2)) {
            targetSets.move(row, row + 1);
            endMoveRows();
            if (m_rootNodes[node.rootRow].isProject) {
                Q_EMIT projectTargetChanged(dir);
            }
        } else {
            Q_ASSERT(false);
        }
        return;
    }

    // It is a command-row
    QList<Command> &commands = targetSets[node.targetSetRow].commands;
    if (beginMoveRows(parent, row, row, parent, row + 2)) {
        commands.move(row, row + 1);
        endMoveRows();
        if (m_rootNodes[node.rootRow].isProject) {
            Q_EMIT projectTargetChanged(targetSets[node.targetSetRow].projectBaseDir);
        }
    } else {
        Q_ASSERT(false);
    }
}

static QString toRitchText(const QString &str)
{
    if (str.isEmpty()) {
        return {};
    }
    return u"<p>%1</p>"_s.arg(str.toHtmlEscaped());
}

QVariant TargetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qWarning("Invalid index");
        return {};
    }

    NodeInfo node = modelToNodeInfo(index);
    if (!nodeExists(m_rootNodes, node)) {
        qDebug() << "Node does not exist:" << node;
        return {};
    }

    if (node.isRoot()) {
        if ((role == Qt::DisplayRole || role == Qt::ToolTipRole) && index.column() == 0) {
            return m_rootNodes[node.rootRow].isProject ? i18n("Projects") : i18n("Session");
        } else if (role == RowTypeRole) {
            return RowType::RootRow;
        } else if (role == IsProjectTargetRole) {
            return m_rootNodes[node.rootRow].isProject;
        }
        return {};
    }

    // This is either a TargetSet or a Command
    const TargetSet &targetSet = m_rootNodes[node.rootRow].targetSets[node.targetSetRow];

    if (node.isTargetSet()) {
        // This is a TargetSet node
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            switch (index.column()) {
            case 0:
                return targetSet.name;
            case 1:
                return targetSet.workDir;
            }
            break;
        case Qt::ToolTipRole:
            switch (index.column()) {
            case 0:
                return toRitchText(targetSet.name);
            case 1:
                return toRitchText(targetSet.workDir);
            }
            break;
        case CommandRole:
            if (targetSet.commands.isEmpty()) {
                return {};
            }
            return targetSet.commands[0].buildCmd;
        case CommandNameRole:
            if (targetSet.commands.isEmpty()) {
                return {};
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
            switch (index.column()) {
            case 0:
                return command.name;
            case 1:
                return command.buildCmd;
            case 2:
                return command.runCmd;
            }
            break;
        case Qt::ToolTipRole:
            switch (index.column()) {
            case 0:
                return toRitchText(command.name);
            case 1:
                return toRitchText(command.buildCmd);
            case 2:
                return toRitchText(command.runCmd);
            }
            break;
        case CommandRole:
            return command.buildCmd;
        case RunCommandRole:
            return command.runCmd;
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

    return {};
}

QVariant TargetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation != Qt::Horizontal) {
        return {};
    }

    if (section == 0) {
        return i18n("Command/Target-set Name");
    }
    if (section == 1) {
        return i18n("Build Command / Working Directory");
    }
    if (section == 2) {
        return i18n("Run Command");
    }
    return {};
}

bool TargetModel::setData(const QModelIndex &itemIndex, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }
    if (!itemIndex.isValid()) {
        qWarning("Invalid index");
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
    QString dir = targetSet.projectBaseDir;

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
            Q_EMIT projectTargetChanged(dir);
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
        return {};
    }

    if (!parent.isValid()) {
        // RootRow Item (Session/Project)
        if (row >= m_rootNodes.size()) {
            return {};
        }
        return createIndex(row, column, InvalidIndex);
    }

    if (parent.column() != 0) {
        // Only column 0 can have children.
        return {};
    }

    if (parent.internalId() == InvalidIndex) {
        // TargetSet node
        int rootRow = parent.row();
        if (rootRow >= m_rootNodes.size() || row >= m_rootNodes.at(rootRow).targetSets.size()) {
            return {};
        }
        return createIndex(row, column, toInternalId(rootRow, -1));
    }

    // This is a command node
    int rootRow = idToRootRow(parent.internalId());
    int targetSetRow = parent.row();
    if (rootRow >= m_rootNodes.size() || targetSetRow >= m_rootNodes.at(rootRow).targetSets.size()) {
        return {};
    }
    const TargetSet &tgSet = m_rootNodes.at(rootRow).targetSets.at(targetSetRow);
    if (row >= tgSet.commands.size()) {
        return {};
    }
    return createIndex(row, column, toInternalId(rootRow, targetSetRow));
}

QModelIndex TargetModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return {};
    }

    if (child.internalId() == InvalidIndex) {
        // child is a RootRow node -> invalid parent
        return {};
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
    obj[QStringLiteral("loaded_via_cmake")] = set.loadedViaCMake;
    obj[QStringLiteral("cmake_config")] = set.cmakeConfigName;

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
        return {};
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

QJsonObject TargetModel::projectTargetsToJsonObj(const QString &projectBaseDir) const
{
    const auto idx = projectRootIndex();
    if (!idx.isValid()) {
        qWarning("Unexpected invalid project root node");
        return {};
    }
    const auto node = modelToNodeInfo(idx);
    Q_ASSERT(node.isRoot());

    QJsonObject obj;
    QJsonArray sets;
    for (const TargetSet &set : std::as_const(m_rootNodes[node.rootRow].targetSets)) {
        if (set.projectBaseDir == projectBaseDir) {
            sets << toJson(set);
        }
    }
    if (!sets.isEmpty()) {
        obj[QStringLiteral("target_sets")] = sets;
    }
    return obj;
}

QString TargetModel::indexToJson(const QModelIndex &modelIndex) const
{
    QJsonDocument doc(indexToJsonObj(modelIndex));
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QModelIndex TargetModel::insertAfter(const QModelIndex &modelIndex, const QString &jsonStr, const QString &projectBaseDir)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning("Could not parse the provided Json");
        return {};
    }
    return insertAfter(modelIndex, doc.object(), projectBaseDir);
}

QModelIndex TargetModel::insertAfter(const QModelIndex &modelIndex, const QJsonObject &obj, const QString &projectBaseDir)
{
    QModelIndex currentIndex = modelIndex;
    if (obj.contains(QStringLiteral("target_sets"))) {
        const QJsonArray sets = obj[QStringLiteral("target_sets")].toArray();
        for (const auto &set : sets) {
            currentIndex = insertAfter(currentIndex, set.toObject(), projectBaseDir);
            if (!currentIndex.isValid()) {
                qWarning("Failed to insert targetset");
                return {};
            }
        }
    } else if (obj.contains(QStringLiteral("targets"))) {
        QString dir = obj[QStringLiteral("directory")].toString();
        QString name = obj[QStringLiteral("name")].toString();
        currentIndex = insertTargetSetAfter(currentIndex, name, dir, false, QString(), projectBaseDir);
        QModelIndex setIndex = currentIndex;
        const QJsonArray targets = obj[QStringLiteral("targets")].toArray();
        for (const auto target : targets) {
            currentIndex = insertAfter(currentIndex, target.toObject(), projectBaseDir);
            if (!currentIndex.isValid()) {
                qWarning("Failed to insert target");
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
