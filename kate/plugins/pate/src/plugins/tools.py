# -*- coding: utf-8 -*-
#
# Copyright 2010-2012 by Alex Trubov <i.zaufi@gmail.com>
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
# Here is a short list of plugins in this file:
#
#   Insert Color (Meta+Shift+C)
#       open color choose dialog and insert selected color as #hex string
#

'''Plugins for general use'''

import kate

from PyQt4 import QtGui
from PyKDE4 import kdeui
from PyKDE4.ktexteditor import KTextEditor

from libkatepate import common

_LEFT_COLOR_BOUNDARY = set(' \t"\';[]{}():/\\,+=!?%^|&*~`')
_RIGHT_COLOR_BOUNDARY = set(' \t"\';[]{}():/\\,+=!?%^|&*~`#')

@kate.action('Insert Color', shortcut='Meta+Shift+C', icon='color', menu='Tools')
def insertColor():
    ''' Insert/edit #color using color chooser dialog

        If cursor positioned in a #color 'word', this action will edit it, otherwise
        a new #color will be inserted into a document.
    '''
    document = kate.activeDocument()
    view = kate.activeView()

    if not view.selection():
        cursor = view.cursorPosition()
        colorRange = common.getBoundTextRangeSL(_LEFT_COLOR_BOUNDARY, _RIGHT_COLOR_BOUNDARY, cursor, document)
    else:
        colorRange = view.selectionRange()

    currentColor = document.text(colorRange)
    color = QtGui.QColor(currentColor)
    result = kdeui.KColorDialog.getColor(color)
    if result == kdeui.KColorDialog.Accepted:
        colorStr = color.name()
        document.startEditing()
        document.replaceText(colorRange, colorStr)
        document.endEditing()
        if view.selection():
            startPos = colorRange.start()
            endPos = KTextEditor.Cursor(startPos.line(), startPos.column() + len(colorStr))
            view.setSelection(KTextEditor.Range(startPos, endPos))

# kate: indent-width 4;
