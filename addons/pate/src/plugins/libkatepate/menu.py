# -*- coding: utf-8 -*-
# Copyright (c) 2013 by Pablo Martín <goinnn@gmail.com>
#
# This software is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this software.  If not, see <http://www.gnu.org/licenses/>.

# This code originally was in this repository:
# <https://github.com/goinnn/Kate-plugins/blob/master/kate_plugins/kate_core_plugins.py>

import kate

from PyQt4 import QtGui


def separated_menu(menu_parent_slug):
    menu_parent = findMenu(menu_parent_slug)
    action = QtGui.QAction('', None)
    menu_parent.insertSeparator(action)


def move_menu_submenu(menu_slug, submenu_slug):
    menu = findMenu(menu_slug)
    submenu = findMenu(submenu_slug)
    action_menubar = _action_of_menu(submenu)
    action = QtGui.QAction(action_menubar.text(), submenu)
    action.setObjectName(action_menubar.text())
    action.setMenu(submenu)
    if action_menubar:
        action_menubar.deleteLater()
    menu.addAction(action)


def findMenu(menu_parent_slug):
    window = kate.mainWindow()
    for menu in window.findChildren(QtGui.QMenu):
        if menu.objectName() == menu_parent_slug:
            return menu
    return None


def _action_of_menu(menu):
    actions = [action for action in menu.parent().actions() if action.menu() and action.menu().objectName() == menu.objectName()]
    if len(actions) == 1:
        return actions[0]
    return None
