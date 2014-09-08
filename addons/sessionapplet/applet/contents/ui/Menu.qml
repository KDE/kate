/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Joseph Wenninger <jowenn@kde.org>

Based on the clipboard applet:
Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
import QtQuick 2.0
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaExtras.ScrollArea {
    id: menu
    property alias view: menuListView
    property alias model: menuListView.model
    signal itemSelected(string uuid)
    signal remove(string uuid)
    signal newSession(string sessionName)

    ListView {
        id: menuListView
        focus: true

        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        highlight: PlasmaComponents.Highlight {
            anchors.bottomMargin: -listMargins.bottom
            y: 1
        }
        highlightMoveDuration: 0
        highlightResizeDuration: 0
        currentIndex: -1

        delegate: KateSessionsItemDelegate {
            width: menuListView.width

            onItemSelected: menu.itemSelected(uuid)
            onRemove: menu.remove(uuid)
            onNewSession:menu.newSession(sessionname)
        }
    }
}
