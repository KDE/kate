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

#include "selecttargetdialog.h"


#include <QCoreApplication>
#include <QGridLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>

#include <KLocalizedString>


SelectTargetDialog::SelectTargetDialog(QList<KateBuildView::TargetSet>& targetSets, QWidget* parent)
:QDialog(parent)
,m_currentTargetSet(0)
,m_targetName(0)
,m_targetsList(0)
,m_command(0)
,m_targetSets(targetSets)
,m_targets(0)
{
    //KF5 FIXME setButtons( KDialog::Ok | KDialog::Cancel);

    QWidget* container = new QWidget();

    QLabel* filterLabel = new QLabel(i18n("Target:"));
    m_targetName = new QLineEdit();
    m_targetsList = new QListWidget();

    QLabel* setLabel = new QLabel(i18n("from"));
    m_currentTargetSet = new QComboBox();
    for (int i=0; i<m_targetSets.size(); i++) {
        m_currentTargetSet->addItem(m_targetSets.at(i).name);
    }

    QLabel* commandLabel = new QLabel(i18n("Command:"));
    m_command = new QLabel();

    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(m_targetName);
    filterLayout->addWidget(setLabel);
    filterLayout->addWidget(m_currentTargetSet);

    QHBoxLayout* commandLayout = new QHBoxLayout();
    commandLayout->addWidget(commandLabel);
    commandLayout->addWidget(m_command);
    commandLayout->setAlignment(Qt::AlignLeft);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    mainLayout->addLayout(filterLayout);
    mainLayout->addWidget(m_targetsList);
    mainLayout->addLayout(commandLayout);

    container->setLayout(mainLayout);

    QVBoxLayout *dialogLayout = new QVBoxLayout;
    setLayout(dialogLayout);
    dialogLayout->addWidget(container);

    connect(m_currentTargetSet, SIGNAL(currentIndexChanged(int)), this, SLOT(slotTargetSetSelected(int)));
    connect(m_targetName, SIGNAL(textEdited(const QString&)), this, SLOT(slotFilterTargets(const QString&)));
    connect(m_targetsList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(accept()));
    connect(m_targetsList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(slotCurrentItemChanged(QListWidgetItem*)));
    m_targetName->installEventFilter(this);
    m_targetsList->installEventFilter(this);

    this->setFocusProxy(m_targetName);
    m_targetName->setFocus();
}


void SelectTargetDialog::slotTargetSetSelected(int index)
{
    setTargetSet(m_targetSets.at(index).name);
}


void SelectTargetDialog::setTargetSet(const QString& name)
{
    m_targets = NULL;
    m_allTargets.clear();
    m_targetsList->clear();
    m_command->setText(QString());
    m_targetName->clear();

    for (int i=0; i<m_targetSets.size(); i++) {
        if (m_targetSets.at(i).name == name) {
            m_currentTargetSet->setCurrentIndex(i);
            setTargets(m_targetSets.at(i).targets);
            return;
        }
    }
}


void SelectTargetDialog::slotFilterTargets(const QString& filter)
{
    QStringList filteredTargets;
    if (filter.isEmpty()) {
        filteredTargets = m_allTargets;
    }
    else {
        filteredTargets = m_allTargets.filter(filter, Qt::CaseInsensitive);
    }
    m_targetsList->clear();
    m_targetsList->addItems(filteredTargets);
    if (filteredTargets.size() > 0) {
        m_targetsList->item(0)->setSelected(true);
        m_targetsList->setCurrentItem(m_targetsList->item(0));
    }
}


void SelectTargetDialog::setTargets(const std::map<QString, QString>& targets)
{
    m_targets = &targets;
    m_allTargets.clear();

    for(std::map<QString, QString>::const_iterator tgtIt = targets.begin(); tgtIt != targets.end(); ++tgtIt) {
        m_allTargets << tgtIt->first;
    }

    slotFilterTargets(QString());
}


void SelectTargetDialog::slotCurrentItemChanged(QListWidgetItem* currentItem)
{
    QString command;

    if (currentItem && m_targets) {
        std::map<QString, QString>::const_iterator tgtIt = m_targets->find(currentItem->text());
        if (tgtIt != m_targets->end()) {
            command = tgtIt->second;
        }
    }

    m_command->setText(command);
}


QString SelectTargetDialog::selectedTarget() const
{
    if (m_targetsList->currentItem() == 0) {
        return m_targetName->text();
    }
    return m_targetsList->currentItem()->text();
}


bool SelectTargetDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type()==QEvent::KeyPress) {
        QKeyEvent *keyEvent=static_cast<QKeyEvent*>(event);
        if (obj==m_targetName) {
            const bool forward2list = (keyEvent->key()==Qt::Key_Up)
            || (keyEvent->key()==Qt::Key_Down)
            || (keyEvent->key()==Qt::Key_PageUp)
            || (keyEvent->key()==Qt::Key_PageDown);
            if (forward2list) {
                QCoreApplication::sendEvent(m_targetsList,event);
                return true;
            }
        }
        else {
            const bool forward2input = (keyEvent->key()!=Qt::Key_Up)
            && (keyEvent->key()!=Qt::Key_Down)
            && (keyEvent->key()!=Qt::Key_PageUp)
            && (keyEvent->key()!=Qt::Key_PageDown)
            && (keyEvent->key()!=Qt::Key_Tab)
            && (keyEvent->key()!=Qt::Key_Backtab);
            if (forward2input) {
                QCoreApplication::sendEvent(m_targetName,event);
                return true;
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

// kate: space-indent on; indent-width 4; replace-tabs on;
