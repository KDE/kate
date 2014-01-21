#ifndef SELECT_TARGET_DIALOG_H
#define SELECT_TARGET_DIALOG_H

// Description: dialog for selecting a target to build
//
// Copyright (c) 2013 Alexander Neundorf <neundorf@kde.org>
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License version 2 as published by the Free Software Foundation.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with this library; see the file COPYING.LIB.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA 02110-1301, USA.


#include <QDialog>

#include "plugin_katebuild.h"

#include <QStringList>

#include <map>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;


class SelectTargetDialog : public QDialog
{
    Q_OBJECT
    public:
        SelectTargetDialog(QList<KateBuildView::TargetSet>& targetSets, QWidget* parent);
        void setTargetSet(const QString& name);

        QString selectedTarget() const;

    protected:
        virtual bool eventFilter(QObject *obj, QEvent *event);

    private Q_SLOTS:
        void slotFilterTargets(const QString& filter);
        void slotCurrentItemChanged(QListWidgetItem* currentItem);
        void slotTargetSetSelected(int index);

    private:
        void setTargets(const std::map<QString, QString>& _targets);

        QStringList m_allTargets;

        QComboBox* m_currentTargetSet;
        QLineEdit* m_targetName;
        QListWidget* m_targetsList;
        QLabel* m_command;

        QList<KateBuildView::TargetSet>& m_targetSets;
        const std::map<QString, QString>* m_targets;
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;
