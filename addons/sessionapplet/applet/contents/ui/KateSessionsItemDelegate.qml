/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Joseph Wenninger <jowenn@kde.org>

Based on the clipboard applet:
Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>
Copyright     2014 Sebastian Kügler <sebas@kde.org>

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
import QtQuick.Layouts 1.1
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons
import org.kde.plasma.core 2.0 as PlasmaCore


PlasmaComponents.ListItem {
    id: menuItem

    signal itemSelected(string uuid)
    signal newSession(string sessionname)
    signal remove(string uuid)
    
    property bool showInput: false
    
    width: parent.width - units.gridUnit * 2
    height: Math.max(Math.max(label.height, toolButtonsLayout.implicitHeight),sessionnameditlayout.implicitHeight) + 2 * units.smallSpacing

    x: -listMargins.left

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            if (TypeRole!=1)
                menuItem.itemSelected(UuidRole);
            else {
                showInput=true; 
            }
        }
        onEntered: menuListView.currentIndex = index
        onExited: menuListView.currentIndex = -1

        Item {
            id: label
            height: childrenRect.height
            anchors {
                left: parent.left
                leftMargin: units.gridUnit / 2
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            PlasmaComponents.Label {
                height: implicitHeight
                anchors {
                    left: parent.left
                    right: parent.right
                    rightMargin: units.gridUnit * 2
                    leftMargin: units.gridUnit * 2
                }
                maximumLineCount: 1
                text: DisplayRole.trim()
                visible: !showInput // TypeRole: 0: Text, 1: Image, 2: Url
                elide: Text.ElideRight
                wrapMode: Text.Wrap
            }

            PlasmaCore.IconItem {
                width: units.iconSizes.small
                height: width
                z: 900
                source: DecorationRole
                enabled: true
                visible: true
            }

        }

        RowLayout {
                id:sessionnameditlayout
                visible:showInput
                height: implicitHeight
                anchors {
                    left: parent.left
                    right: parent.right
                    rightMargin: 0
                    leftMargin: units.gridUnit * 2
                }
                /*Layout.fillWidth: true*/
                Item {
                    width: units.gridUnit / 2 - parent.spacing
                    height: 1
                }
                PlasmaComponents.TextField {
                    id: sessionname
                    placeholderText: i18n("Session name")
                    clearButtonShown: true
                    Layout.fillWidth: true
                    
                }
                PlasmaComponents.ToolButton {
                    iconSource: "dialog-ok"
                    tooltip: i18n("Create new session and start Kate")
                    enabled: sessionname.text.replace(/^\s+|\s+$/g, '').length>0
                    onClicked: {menuItem.newSession(sessionname.text.replace(/^\s+|\s+$/g, '')); showInput=false;}
                }
                PlasmaComponents.ToolButton {
                    iconSource: "dialog-cancel"
                    tooltip: i18n("Cancel session creation")
                    onClicked: showInput=false
                }
        }
        
        RowLayout {
            id: toolButtonsLayout
            anchors {
                right: label.right
                verticalCenter: parent.verticalCenter
            }

            PlasmaComponents.ToolButton {
                iconSource: "edit-delete"
                tooltip: i18n("Delete session")
                onClicked: menuItem.remove(UuidRole)
            }

            Component.onCompleted: {
                toolButtonsLayout.visible = Qt.binding(function () { return (TypeRole==2) && (menuListView.currentIndex == index); });
            }
        }
    }
}
