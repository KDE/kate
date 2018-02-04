/* This file is part of the KDE project

   Copyright (C) 2014 Dominik Haumann <dhaumann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "tabswitchertreeview.h"
#include "tabswitcher.h"

#include <QKeyEvent>

TabSwitcherTreeView::TabSwitcherTreeView()
    : QListView()
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setUniformItemSizes(true);
    setTextElideMode(Qt::ElideMiddle);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void TabSwitcherTreeView::keyReleaseEvent(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Control) {
        emit itemActivated(selectionModel()->currentIndex());
        event->accept();
        hide();
    } else {
        QListView::keyReleaseEvent(event);
    }
}

void TabSwitcherTreeView::keyPressEvent(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        hide();
    } else {
        QListView::keyPressEvent(event);
    }
}
