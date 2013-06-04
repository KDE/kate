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

#include "edittargetdialog.h"

#include <klocale.h>


#include <QGridLayout>


EditTargetDialog::EditTargetDialog(const QStringList& existingTargets, QWidget* parent)
:KDialog()
,m_existingTargets(existingTargets)
,m_nameLabel(0)
,m_name(0)
,m_cmdLabel(0)
,m_cmd(0)
{
    setButtons(KDialog::Ok | KDialog::Cancel);

    m_nameLabel = new QLabel(i18n("Name"), this);
    m_name = new KLineEdit(this);
    m_name->setClearButtonShown(true);

    m_cmdLabel = new QLabel(i18n("Command"), this);
    m_cmd = new KLineEdit(this);
    m_cmd->setClearButtonShown(true);
    m_cmd->setMinimumWidth(300);


    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(m_nameLabel, 0, 0);
    layout->addWidget(m_name, 0, 1, 1, 2);

    layout->addWidget(m_cmdLabel, 1, 0);
    layout->addWidget(m_cmd, 1, 1, 1, 2);

    layout->setRowStretch(2, 1);

    QWidget* container = new QWidget(this);
    container->setLayout(layout);

    setMainWidget(container);

    connect(m_name, SIGNAL(textEdited(const QString&)), this, SLOT(checkTargetName(const QString&)));

    m_name->setFocus();
}


void EditTargetDialog::setTargetCommand(const QString& cmd)
{
    m_cmd->setText(cmd);
}


void EditTargetDialog::setTargetName(const QString& name)
{
    m_name->setText(name);
    m_initialTargetName = name;
}


QString EditTargetDialog::targetCommand() const
{
    return m_cmd->text();
}


QString EditTargetDialog::targetName() const
{
    return m_name->text();
}


void EditTargetDialog::checkTargetName(const QString& name)
{
    bool enableOk = true;

    enableOk = (enableOk && !name.isEmpty());

    if (name != m_initialTargetName) {
        enableOk = (enableOk && !(m_existingTargets.contains(name)));
    }
    enableButtonOk(enableOk);
}


// kate: space-indent on; indent-width 4; replace-tabs on;
