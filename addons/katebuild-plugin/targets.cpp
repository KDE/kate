//
// Description: Widget for configuring build targets
//
// Copyright (c) 2011-2014 Kåre Särs <kare.sars@iki.fi>
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
#include <klocalizedstring.h>
//#include <QApplication>
#include <QHeaderView>

TargetsUi::TargetsUi(QWidget *parent):
QWidget(parent)
{

    targetLabel = new QLabel(i18n("Target set"), this);

    targetCombo = new QComboBox(this);
    targetCombo->setEditable(true);
    targetCombo->setInsertPolicy(QComboBox::InsertAtCurrent);
    connect(targetCombo, SIGNAL(editTextChanged(QString)), this, SLOT(editTarget(QString)));
    targetLabel->setBuddy(targetCombo);

    newTarget = new QToolButton(this);
    newTarget->setToolTip(i18n("Create new set of targets"));
    newTarget->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));

    copyTarget = new QToolButton(this);
    copyTarget->setToolTip(i18n("Copy set of targets"));
    copyTarget->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));

    deleteTarget = new QToolButton(this);
    deleteTarget->setToolTip(i18n("Delete current set of targets"));
    deleteTarget->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));

    dirLabel = new QLabel(i18n("Working directory"), this);
    buildDir = new QLineEdit(this);
    buildDir->setToolTip(i18n("Leave empty to use the directory of the current document. "));
    // KF5 FIXME buildDir->setClearButtonShown(true);
    browse = new QToolButton(this);
    browse->setIcon(QIcon::fromTheme(QStringLiteral("inode-directory")));

//    quickCmd->setToolTip(i18n("Use:\n\"%f\" for current file\n\"%d\" for directory of current file\n\"%n\" for current file name without suffix"));

    dirLabel->setBuddy(buildDir);

    targetsList = new QTableWidget(0, 4, this);
    targetsList->setAlternatingRowColors(true);
    targetsList->setWordWrap(false);
    targetsList->setShowGrid(false);
    targetsList->setSelectionMode(QAbstractItemView::SingleSelection);
    targetsList->setSelectionBehavior(QAbstractItemView::SelectItems); //SelectRows);
    QStringList headerLabels;
    headerLabels << QStringLiteral("Def") << QStringLiteral("Clean") << QStringLiteral("Name") << QStringLiteral("Command");
    targetsList->setHorizontalHeaderLabels(headerLabels);
    targetsList->verticalHeader()->setVisible(false);

    addButton = new QToolButton(this);
    addButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    addButton->setToolTip(i18n("Add new target"));

    deleteButton = new QToolButton(this);
    deleteButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    deleteButton->setToolTip(i18n("Delete selected target"));

    buildButton = new QToolButton(this);
    buildButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok")));
    buildButton->setToolTip(i18n("Build selected target"));
    
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
    if (m_useBottomLayout && (size().height() > m_widgetsHeight) && (size().width() < m_widgetsHeight * 2.5))
    {
        delete layout();
        setSideLayout();
        m_useBottomLayout = false;
    }
    else if (!m_useBottomLayout && ((size().height() < m_widgetsHeight) || (size().width() > m_widgetsHeight * 2.5)))
    {
        delete layout();
        setBottomLayout();
        m_useBottomLayout = true;
    }
}

void TargetsUi::setSideLayout()
{
    QHBoxLayout* tLayout = new QHBoxLayout();
    tLayout->addWidget(targetCombo, 1);
    tLayout->addWidget(newTarget, 0);
    tLayout->addWidget(copyTarget, 0);
    tLayout->addWidget(deleteTarget, 0);
    tLayout->setContentsMargins(0,0,0,0);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(deleteButton);
    buttonsLayout->addWidget(buildButton);

    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(targetLabel, 0, 0, 1, 4);
    layout->addLayout(tLayout, 1, 0, 1, 4);

    layout->addWidget(dirLabel, 2, 0, Qt::AlignLeft);
    layout->addWidget(buildDir, 3, 0, 1, 3);
    layout->addWidget(browse, 3, 3);

    layout->addWidget(targetsList, 4, 0, 1, 4);
    layout->addLayout(buttonsLayout, 5, 0,  1, 4);

    layout->addItem(new QSpacerItem(1, 1), 8, 0);
    layout->setColumnStretch(0, 1);
    layout->setRowStretch(4, 1);
}

void TargetsUi::setBottomLayout()
{
    QHBoxLayout* tLayout = new QHBoxLayout();
    tLayout->addWidget(targetLabel);
    tLayout->addWidget(targetCombo, 1);
    tLayout->addWidget(newTarget, 0);
    tLayout->addWidget(copyTarget, 0);
    tLayout->addWidget(deleteTarget, 0);
    tLayout->setContentsMargins(0,0,0,0);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(dirLabel);
    buttonsLayout->addWidget(buildDir);
    buttonsLayout->addWidget(browse);

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(deleteButton);
    buttonsLayout->addWidget(buildButton);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(tLayout);
    layout->addWidget(targetsList);
    layout->addLayout(buttonsLayout);
}

void TargetsUi::editTarget(const QString &text)
{
    int curPos = targetCombo->lineEdit()->cursorPosition();
    targetCombo->setItemText(targetCombo->currentIndex(), text);
    targetCombo->lineEdit()->setCursorPosition(curPos);
}

#include "targets.moc"
