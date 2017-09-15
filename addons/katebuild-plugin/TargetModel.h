/***************************************************************************
 *   This file is part of Kate build plugin                                *
 *   Copyright 2014 Kåre Särs <kare.sars@iki.fi>                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/


#ifndef TargetModel_h
#define TargetModel_h

#include <QAbstractItemModel>
#include <QByteArray>

class TargetModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    struct TargetSet {
        TargetSet(const QString &_name, const QString &_workDir);
        QString name;
        QString workDir;
        QString defaultCmd;
        QList<QPair<QString, QString> > commands;
    };

    TargetModel(QObject *parent = nullptr);
    ~TargetModel() override;

    /** This function sets the default command for a target set */
    void setDefaultCmd(int rootRow, const QString &defCmd);

public Q_SLOTS:

    /** This function clears all the target-sets */
    void clear();

    /** This function adds a target set and returns the row number of the newly
     * inserted row */
    int addTargetSet(const QString &setName, const QString &workDir);

    /** This function adds a new command to a target-set and returns the model index */
    QModelIndex addCommand(int rootRow, const QString &cmdName, const QString &command);

    /** This function copies the target(-set) the model index points to and returns
     * the model index of the copy. */
    QModelIndex copyTargetOrSet(const QModelIndex &index);

    /** This function returns the model index of the default command of the target-set */
    QModelIndex defaultTarget(const QModelIndex &index);

    /** This function deletes the index */
    void deleteItem(const QModelIndex &index);

    /** This function deletes the target-set with the same name */
    void deleteTargetSet(const QString &targetSet);

    const QList<TargetSet> targetSets() const { return m_targets; }

    const QString command(const QModelIndex &itemIndex) const;
    const QString cmdName(const QModelIndex &itemIndex) const;
    const QString workDir(const QModelIndex &itemIndex) const;
    const QString targetName(const QModelIndex &itemIndex) const;
    
Q_SIGNALS:

public:
    static const quint32 InvalidIndex = 0xFFFFFFFF;
    // Model-View model functions
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:

    QList<TargetSet> m_targets;
};

#endif
