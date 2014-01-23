# -*- coding: utf-8 -*-
#
# Kate/Pâté color plugins
# Copyright 2013 by Alex Turbov <i.zaufi@gmail.com>
# Copyright 2013 by Phil Schaf
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
#

from PyQt4.QtCore import Qt


def setTooltips(rows, columns, colors):
    '''Set tool tips for color cells in a KColorCells widget'''
    stop = False
    for r in range(0, rows):
        for c in range(0, columns):
            item = colors.item(r, c)
            if item is None:
                stop = True
                break
            color = item.data(Qt.BackgroundRole)
            if color.isValid():
                item.setToolTip(color.name())
        if stop:
            break
