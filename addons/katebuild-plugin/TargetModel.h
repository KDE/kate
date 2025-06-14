/***************************************************************************
 *   This file is part of Kate build plugin                                *
 *   SPDX-FileCopyrightText: 2014 Kåre Särs <kare.sars@iki.fi>             *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include <QAbstractItemModel>
#include <QByteArray>

#include <limits>

class QJsonObject;

class TargetModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum RowType {
        RootRow,
        TargetSetRow,
        CommandRow,
    };
    Q_ENUM(RowType)

    enum TargetRoles {
        CommandRole = Qt::UserRole,
        RunCommandRole,
        CommandNameRole,
        WorkDirRole,
        SearchPathsRole,
        TargetSetNameRole,
        RowTypeRole,
        IsProjectTargetRole,
    };
    Q_ENUM(TargetRoles)

    explicit TargetModel(QObject *parent = nullptr);
    ~TargetModel() override;

    /** This function clears all the target-sets */
    void clear(bool setSessionFirst);

    /** This function returns the root item for the Session TargetSets */
    QModelIndex sessionRootIndex() const;

    /** This function returns the root item for the Project TargetSets */
    QModelIndex projectRootIndex() const;

    bool validTargetsJson(const QString &jsonStr) const;

    /** This function returns the menu node as a Json object */
    QJsonObject indexToJsonObj(const QModelIndex &modelIndex) const;
    /** This function returns the TargetSet node for the given projectBaseDir **/
    QJsonObject projectTargetsToJsonObj(const QString &projectBaseDir) const;
    QString indexToJson(const QModelIndex &modelIndex) const;

    QModelIndex insertAfter(const QModelIndex &modelIndex, const QString &jsonStr, const QString &projectBaseDir);
    QModelIndex insertAfter(const QModelIndex &modelIndex, const QJsonObject &jsonObj, const QString &projectBaseDir);

public Q_SLOTS:

    /** This function insert a target set and returns the model-index of the newly
     * inserted target-set */
    QModelIndex insertTargetSetAfter(const QModelIndex &beforeIndex,
                                     const QString &setName,
                                     const QString &workDir,
                                     bool loadedViaCMake = false,
                                     const QString &cmakeConfig = QString(),
                                     const QString &projectBaseDir = QString());

    /** This function adds a new command to a target-set and returns the model index */
    QModelIndex addCommandAfter(const QModelIndex &beforeIndex, const QString &cmdName, const QString &buildCmd, const QString &runCmd);

    /** This function deletes the index */
    void deleteItem(const QModelIndex &index);

    /** This function deletes the project target-sets except the keep-list
     * @param keep is a list of the baseDirs to keep
     */
    void deleteProjectTargetsExcept(const QStringList &keep = QStringList());

    void deleteProjectTargets(const QString &baseDir);

    void moveRowUp(const QModelIndex &index);
    void moveRowDown(const QModelIndex &index);

Q_SIGNALS:
    void projectTargetChanged(const QString &projectBaseDir);

public:
    static constexpr quintptr InvalidIndex = std::numeric_limits<quintptr>::max();
    // Model-View model functions
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    struct Command {
        QString name;
        QString buildCmd;
        QString runCmd;
    };

    struct TargetSet {
        TargetSet(const QString &_name, const QString &_workDir, bool _loadedViaCMake, const QString &_cmakeConfigName = QString());
        /** The name of this target set */
        QString name;
        /** The working directory */
        QString workDir;
        /** The list of commands in this TargetSet */
        QList<Command> commands;
        /** Was this TargetSet loaded via cmake */
        bool loadedViaCMake = false;
        /** CMake build config name */
        QString cmakeConfigName;
        /** If the TargetSet belongs to a project, this field will contain the project base dir */
        QString projectBaseDir;
    };

    struct RootNode {
        /** Is this the Project root node? We have two root nodes, Projects and Session */
        bool isProject = false;
        /** The TargetSet in this root node */
        QList<TargetSet> targetSets;
    };

private:
    // TODO: Make this a c array of size 2
    QList<RootNode> m_rootNodes;
};
