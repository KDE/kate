/********************************************************************
This file is part of the KDE project.

SPDX-FileCopyrightText: 2014 Joseph Wenninger <jowenn@kde.org>

Based on the clipboard applet:
SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

SPDX-License-Identifier: GPL-2.0-or-later
*********************************************************************/
import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: main
    width: (Plasmoid.formFactor==PlasmaCore.Types.Planar)? units.gridUnit * 14 : undefined
    height: (Plasmoid.formFactor==PlasmaCore.Types.Planar)? units.gridUnit * 16: undefined
    
    Plasmoid.switchWidth: units.gridUnit * 11
    Plasmoid.switchHeight: units.gridUnit * 11    
    Plasmoid.status: PlasmaCore.Types.ActiveStatus
    Plasmoid.toolTipMainText: i18n("Kate Sessions")
    Plasmoid.icon: "kate"


    Component.onCompleted: {
        plasmoid.removeAction("configure");
    }

    PlasmaCore.DataSource {
        id: clipboardSource
        property bool editing: false;
        engine: "org.kde.plasma.katesessions"
        connectedSources: "katesessions"
        function service(uuid, op) {
            var service = clipboardSource.serviceForSource(uuid);
            var operation = service.operationDescription(op);
            return service.startOperationCall(operation);
        }
        function newSession(sessionName) {
            var service = clipboardSource.serviceForSource("");
            var operation = service.operationDescription("newSession");
            operation.sessionName=sessionName;
            return service.startOperationCall(operation);
        }
        
    }

    Plasmoid.fullRepresentation: Item {

        id: dialogItem
        Layout.minimumWidth: units.gridUnit * 12
        Layout.minimumHeight: units.gridUnit * 12

        focus: true

        property alias listMargins: listItemSvg.margins

        PlasmaCore.FrameSvgItem {
            id : listItemSvg
            imagePath: "widgets/listitem"
            prefix: "normal"
            visible: false
        }

        Keys.onPressed: {
            switch(event.key) {
                case Qt.Key_Up: {
                    clipboardMenu.view.decrementCurrentIndex();
                    event.accepted = true;
                    break;
                }
                case Qt.Key_Down: {
                    clipboardMenu.view.incrementCurrentIndex();
                    event.accepted = true;
                    break;
                }
                case Qt.Key_Enter:
                case Qt.Key_Return: {
                    if (clipboardMenu.view.currentIndex >= 0) {
                        var uuid = clipboardMenu.model.get(clipboardMenu.view.currentIndex).UuidRole
                        if (uuid) {
                            clipboardSource.service(uuid, "invoke")
                            clipboardMenu.view.currentIndex = 0
                        }
                    }
                    break;
                }
                case Qt.Key_Escape: {
                    if (filter.text == "") {
                        plasmoid.expanded = false;
                    } else {
                        filter.text = "";
                    }
                    event.accepted = true;
                    break;
                }
                default: { // forward key to filter
                    // filter.text += event.text wil break if the key is backspace
                    if (event.key == Qt.Key_Backspace && filter.text == "") {
                        return;
                    }
                    if (event.text != "" && !filter.activeFocus) {
                        clipboardMenu.view.currentIndex = -1
                        if (event.text == "v" && event.modifiers & Qt.ControlModifier) {
                            filter.paste();
                        } else {
                            filter.text = "";
                            filter.text += event.text;
                        }
                        filter.forceActiveFocus();
                        event.accepted = true;
                    }
                }
            }
        }
        ColumnLayout {
            anchors.fill: parent
            RowLayout {
                Layout.fillWidth: true
                Item {
                    width: units.gridUnit / 2 - parent.spacing
                    height: 1
                }
                PlasmaComponents.TextField {
                    id: filter
                    placeholderText: i18n("Search")
                    clearButtonShown: true
                    Layout.fillWidth: true
                }
            }
            Menu {
                id: clipboardMenu
                model: PlasmaCore.SortFilterModel {
                    sourceModel: clipboardSource.models.katesessions
                    filterRole: "DisplayRole"
                    filterRegExp: filter.text
                }
                Layout.fillWidth: true
                Layout.fillHeight: true
                onItemSelected: {
                    clipboardSource.service(uuid, "invoke")
                    plasmoid.expanded = false;
                }
                onRemove: clipboardSource.service(uuid, "remove")
                onNewSession:clipboardSource.newSession(sessionName)
            }
            //NewSessionDialog {
            //    id: newsessiondialog
            //}
        }
    }
}
