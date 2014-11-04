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

#ifndef UrlInserter_H
#define UrlInserter_H

#include <QWidget>
#include <QLineEdit>
#include <QToolButton>
#include <QUrl>

class UrlInserter : public QWidget
{
    Q_OBJECT
public:
    UrlInserter(const QUrl &startUrl, QWidget* parent);
    QLineEdit *lineEdit() {return m_lineEdit;}
    void setReplace(bool replace);

public Q_SLOTS:
    void insertFolder();

private:
    QLineEdit   *m_lineEdit;
    QToolButton *m_toolButton;
    QUrl         m_startUrl;
    bool         m_replace;
};

#endif

