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
#include <klocalizedstring.h>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QDebug>
#include <QCompleter>
#include <QDirModel>

UrlInserter::UrlInserter(const QUrl &startUrl, QWidget* parent): QWidget(parent), m_startUrl(startUrl), m_replace(false)
{
    m_lineEdit = new QLineEdit();
    QCompleter* completer = new QCompleter(m_lineEdit);
    completer->setModel(new QDirModel(QStringList(),
                                      QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Executable,
                                      QDir::Name, m_lineEdit));
    m_lineEdit->setCompleter(completer);

    m_toolButton = new QToolButton();
    m_toolButton->setIcon(QIcon::fromTheme(QStringLiteral("archive-insert-directory")));
    m_toolButton->setToolTip(i18n("Insert path"));


    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_toolButton);
    setFocusProxy(m_lineEdit);
    connect(m_toolButton, &QToolButton::clicked, this, &UrlInserter::insertFolder);
}


void UrlInserter::insertFolder()
{
    QUrl startUrl;
    if (QFileInfo(m_lineEdit->text()).exists()) {
        startUrl.setPath(m_lineEdit->text());
    }
    else {
        startUrl = m_startUrl;
    }
    QString folder = QFileDialog::getExistingDirectory(this, i18n("Select directory to insert"),
                                                       startUrl.path());
    if (!folder.isEmpty()) {
        if (!m_replace) {
            m_lineEdit->insert(folder);
        }
        else {
            m_lineEdit->setText(folder);
        }
    }
}

void UrlInserter::setReplace(bool replace)
{
    m_replace = replace;
}
