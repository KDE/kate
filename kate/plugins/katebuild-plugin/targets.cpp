//
// Description: Widget for configuring build targets
//
// Copyright (c) 2011 Kåre Särs <kare.sars@iki.fi>
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

#include "targets.h"
#include "targets.moc"
#include <klocale.h>

TargetsUi::TargetsUi(QWidget *parent):
QWidget(parent)
{
    targetCombo = new QComboBox(this);
    targetCombo->setEditable(true);
    targetCombo->setInsertPolicy(QComboBox::InsertAtCurrent);
    connect(targetCombo, SIGNAL(editTextChanged(QString)), this, SLOT(editTarget(QString)));
    
    newTarget = new QToolButton(this);
    newTarget->setToolTip(i18n("New"));
    newTarget->setIcon(KIcon("document-new"));
    
    copyTarget = new QToolButton(this);
    copyTarget->setToolTip(i18n("Copy"));
    copyTarget->setIcon(KIcon("edit-copy"));

    deleteTarget = new QToolButton(this);
    deleteTarget->setToolTip(i18n("Delete"));
    deleteTarget->setIcon(KIcon("edit-delete"));
    
    line = new QFrame(this);
    line->setFrameShadow(QFrame::Sunken);
    
    dirLabel = new QLabel(i18n("Working directory"), this);
    buildDir = new KLineEdit(this);
    buildDir->setToolTip(i18n("Leave empty to use the directory of the current document. "));
    buildDir->setClearButtonShown(true);
    browse = new QToolButton(this);
    browse->setIcon(KIcon("inode-directory"));
    
    buildLabel = new QLabel(i18n("Build"), this);
    buildCmds = new QComboBox(this);
    m_addBuildCmd = new QToolButton(this);
    m_addBuildCmd->setIcon(KIcon("document-new"));
    m_delBuildCmd = new QToolButton(this);
    m_delBuildCmd->setIcon(KIcon("edit-delete"));
    connect(m_addBuildCmd, SIGNAL(clicked()), this, SLOT(addBuildCmd()));
    connect(m_delBuildCmd, SIGNAL(clicked()), this, SLOT(delBuildCmd()));
    connect(buildCmds, SIGNAL(editTextChanged(QString)), this, SLOT(editBuildCmd(QString)));
    
    cleanLabel = new QLabel(i18n("Clean"), this);
    cleanCmds = new QComboBox(this);
    m_addCleanCmd = new QToolButton(this);
    m_addCleanCmd->setIcon(KIcon("document-new"));
    m_delCleanCmd = new QToolButton(this);
    m_delCleanCmd->setIcon(KIcon("edit-delete"));
    connect(m_addCleanCmd, SIGNAL(clicked()), this, SLOT(addCleanCmd()));
    connect(m_delCleanCmd, SIGNAL(clicked()), this, SLOT(delCleanCmd()));
    connect(cleanCmds, SIGNAL(editTextChanged(QString)), this, SLOT(editCleanCmd(QString)));
    
    quickLabel = new QLabel(i18n("Quick compile"), this);
    quickCmds = new QComboBox(this);
    quickCmds->setToolTip(i18n("Use:\n\"%f\" for current file\n\"%d\" for directory of current file"));
    m_addQuickCmd = new QToolButton(this);
    m_addQuickCmd->setIcon(KIcon("document-new"));
    m_delQuickCmd = new QToolButton(this);
    m_delQuickCmd->setIcon(KIcon("edit-delete"));
    connect(m_addQuickCmd, SIGNAL(clicked()), this, SLOT(addQuickCmd()));
    connect(m_delQuickCmd, SIGNAL(clicked()), this, SLOT(delQuickCmd()));
    connect(quickCmds, SIGNAL(editTextChanged(QString)), this, SLOT(editQuickCmd(QString)));
    

    dirLabel->setBuddy(buildDir);
    buildLabel->setBuddy(buildCmds);
    cleanLabel->setBuddy(cleanCmds);
    quickLabel->setBuddy(quickCmds);

    buildCmds->setEditable(true);
    cleanCmds->setEditable(true);
    quickCmds->setEditable(true);

    buildCmds->setInsertPolicy(QComboBox::InsertAtCurrent);
    cleanCmds->setInsertPolicy(QComboBox::InsertAtCurrent);
    quickCmds->setInsertPolicy(QComboBox::InsertAtCurrent);
    
    // calculate the approximate height to exceed before going to "Side Layout"
    setSideLayout();
    m_widgetsHeight = sizeHint().height();
    delete layout();
    
    setBottomLayout();
    m_useBottomLayout = true;
}

void TargetsUi::resizeEvent(QResizeEvent *)
{
    // check if the widgets fit in a VBox layout
    if (m_useBottomLayout && (size().height() > m_widgetsHeight))
    {
        delete layout();
        setSideLayout();
        m_useBottomLayout = false;
    }
    else if (!m_useBottomLayout && (size().height() < m_widgetsHeight))
    {
        delete layout();
        setBottomLayout();
        m_useBottomLayout = true;
    }
}

void TargetsUi::setSideLayout()
{
    QGridLayout* layout = new QGridLayout(this);
    QHBoxLayout* tLayout = new QHBoxLayout();
    tLayout->addWidget(targetCombo, 1);
    tLayout->addWidget(newTarget, 0);
    tLayout->addWidget(copyTarget, 0);
    tLayout->addWidget(deleteTarget, 0);
    tLayout->setContentsMargins(0,0,0,0);

    layout->addLayout(tLayout, 0, 0, 1, 4);

    line->setFrameShape(QFrame::HLine);
    layout->addWidget(line, 1, 0, 1, 4);
    
    layout->addWidget(dirLabel, 2, 0, Qt::AlignLeft);
    layout->addWidget(buildDir, 3, 0, 1, 3);
    layout->addWidget(browse, 3, 3);
    
    layout->addWidget(buildLabel, 4, 0, Qt::AlignLeft);
    layout->addWidget(buildCmds, 5, 0, 1, 2);
    layout->addWidget(m_addBuildCmd, 5, 2);
    layout->addWidget(m_delBuildCmd, 5, 3);
    
    layout->addWidget(cleanLabel, 6, 0, Qt::AlignLeft);
    layout->addWidget(cleanCmds, 7, 0, 1, 2);
    layout->addWidget(m_addCleanCmd, 7, 2);
    layout->addWidget(m_delCleanCmd, 7, 3);
    
    layout->addWidget(quickLabel, 8, 0, Qt::AlignLeft);
    layout->addWidget(quickCmds, 9, 0, 1, 2);
    layout->addWidget(m_addQuickCmd, 9, 2);
    layout->addWidget(m_delQuickCmd, 9, 3);
    
    layout->addItem(new QSpacerItem(1, 1), 10, 0);
    layout->setColumnStretch(0, 1);
    layout->setRowStretch(12, 1);
}

void TargetsUi::setBottomLayout()
{
    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(targetCombo, 0, 0);
    
    QHBoxLayout* tLayout = new QHBoxLayout();
    tLayout->addWidget(newTarget, 0);
    tLayout->addWidget(copyTarget, 0);
    tLayout->addWidget(deleteTarget, 0);
    tLayout->setContentsMargins(0,0,0,0);
    
    layout->addLayout(tLayout, 1, 0);
    
    line->setFrameShape(QFrame::VLine);
    layout->addWidget(line, 0, 1, 5, 1);

    layout->addWidget(dirLabel, 0, 2, Qt::AlignRight);
    layout->addWidget(buildDir, 0, 3, 1, 2);
    layout->addWidget(browse, 0, 5);
    
    layout->addWidget(buildLabel, 1, 2, Qt::AlignRight);
    layout->addWidget(buildCmds, 1, 3, 1, 1);
    layout->addWidget(m_addBuildCmd, 1, 4);
    layout->addWidget(m_delBuildCmd, 1, 5);
    
    layout->addWidget(cleanLabel, 2, 2, Qt::AlignRight);
    layout->addWidget(cleanCmds, 2, 3, 1, 1);
    layout->addWidget(m_addCleanCmd, 2, 4);
    layout->addWidget(m_delCleanCmd, 2, 5);
    
    layout->addWidget(quickLabel, 3, 2, Qt::AlignRight);
    layout->addWidget(quickCmds, 3, 3, 1, 1);
    layout->addWidget(m_addQuickCmd, 3, 4);
    layout->addWidget(m_delQuickCmd, 3, 5);
    
    layout->addItem(new QSpacerItem(1, 1), 4, 0 );
    layout->setColumnStretch(3, 1);
    layout->setRowStretch(5, 1);
}

void TargetsUi::addBuildCmd()
{
    buildCmds->addItem(QString());
    buildCmds->setCurrentIndex(buildCmds->count()-1);
}

void TargetsUi::addCleanCmd()
{
    cleanCmds->addItem(QString());
    cleanCmds->setCurrentIndex(cleanCmds->count()-1);
}

void TargetsUi::addQuickCmd()
{
    quickCmds->addItem(QString());
    quickCmds->setCurrentIndex(quickCmds->count()-1);
}


void TargetsUi::delBuildCmd()
{
    buildCmds->removeItem(buildCmds->currentIndex());
}

void TargetsUi::delCleanCmd()
{
    cleanCmds->removeItem(cleanCmds->currentIndex());
}

void TargetsUi::delQuickCmd()
{
    quickCmds->removeItem(quickCmds->currentIndex());
}

void TargetsUi::editTarget(const QString &text)
{
    targetCombo->setItemText(targetCombo->currentIndex(), text);
}

void TargetsUi::editBuildCmd(const QString &text)
{
    buildCmds->setItemText(buildCmds->currentIndex(), text);
}

void TargetsUi::editCleanCmd(const QString &text)
{
    cleanCmds->setItemText(cleanCmds->currentIndex(), text);
}

void TargetsUi::editQuickCmd(const QString &text)
{
    quickCmds->setItemText(quickCmds->currentIndex(), text);
}

