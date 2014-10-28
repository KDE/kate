/***************************************************************************
 *   This file is part of Kate search plugin                               *
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

#include "UrlInserter.h"
#include <klocale.h>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QIcon>

UrlInserter::UrlInserter(QWidget* parent): QWidget(parent)
{
    m_lineEdit = new QLineEdit();
    m_toolButton = new QToolButton();
    m_toolButton->setIcon(QIcon::fromTheme(QStringLiteral("archive-insert-directory")));
    m_toolButton->setToolTip(i18n("Inset path"));


    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_toolButton);
    setFocusProxy(m_lineEdit);
    connect(m_toolButton, SIGNAL(clicked(bool)), this, SLOT(insertFolder()));
}


void UrlInserter::insertFolder()
{
    QString folder = QFileDialog::getExistingDirectory(this, i18n("Select directory to insert"),
                                                       m_lineEdit->text());
    if (!folder.isEmpty()) {
        m_lineEdit->insert(folder);
    }
}

