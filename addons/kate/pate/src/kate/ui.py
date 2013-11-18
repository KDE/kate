# -*- coding: utf-8 -*-
#
# Copyright 2010-2012 by Alex Turbov <i.zaufi@gmail.com>
#
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
#

''' Reusable code for Kate/Pâté plugins: UI elements '''

from PyQt4 import QtCore
from PyQt4.QtCore import QSize
from PyKDE4.kdeui import KPassivePopup, KIcon
from .api import mainWindow


def popup(caption, text, iconName = None, iconSize = 16):
    ''' Show passive popup using native KDE API
    '''
    parentWidget = mainWindow()
    if iconName:
        icon = KIcon (iconName).pixmap(QSize(iconSize, iconSize))
        KPassivePopup.message(caption, text, icon, parentWidget)
    else:
        KPassivePopup.message(caption, text, parentWidget)
