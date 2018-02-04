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

#ifndef KTEXTEDITOR_TABSWITCHER_TREEVIEW_H
#define KTEXTEDITOR_TABSWITCHER_TREEVIEW_H

#include <QListView>

class TabSwitcherTreeView : public QListView
{
    Q_OBJECT

public:
    /**
     * Default constructor.
     */
    TabSwitcherTreeView();

Q_SIGNALS:
    /**
     * This signal is emitted whenever use activates an item through
     * the list view.
     * @note @p selectionIndex is a model index of the selectionModel()
     *       and not of the QListView's model itself.
     */
    void itemActivated(const QModelIndex & selectionIndex);

protected:
    /**
     * Reimplemented for tracking the CTRL key modifier.
     */
    void keyReleaseEvent(QKeyEvent * event) override;

    /**
     * Reimplemented for tracking the ESCAPE key.
     */
    void keyPressEvent(QKeyEvent * event) override;
};

#endif // KTEXTEDITOR_TABSWITCHER_TREEVIEW_H
