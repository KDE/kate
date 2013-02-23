# -*- coding: utf-8 -*-
#
# Kate/Pâté plugins for general use
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

_LEFT_COLOR_BOUNDARY = set(' \t"\';[]{}()<>:/\\,.-_+=!?%^|&*~`')
_RIGHT_COLOR_BOUNDARY = set(' \t"\';[]{}()<>:/\\,.-_+=!?%^|&*~`#')
_INSERT_COLOR_LCC = "insertColor:lastUsedColor"


@kate.action('Insert Color', shortcut='Meta+Shift+C', icon='color', menu='Tools')
def insertColor():
    ''' Insert/edit #color using color chooser dialog

        If cursor positioned in a #color 'word', this action will edit it, otherwise
        a new #color will be inserted into a document.
    '''
    document = kate.activeDocument()
    view = kate.activeView()
    cursor = view.cursorPosition()

    if not view.selection():
        # If no selection, try to get a #color under cursor
        colorRange = common.getBoundTextRangeSL(_LEFT_COLOR_BOUNDARY, _RIGHT_COLOR_BOUNDARY, cursor, document)
    else:
        # Some text selected, just use it as input...
        colorRange = view.selectionRange()

    if colorRange.isValid():
        currentColor = document.text(colorRange)
    else:
        currentColor = kate.configuration[_INSERT_COLOR_LCC]

    color = QtGui.QColor(currentColor)
    # If no text selected (i.e. user don't want to override selection)
    # and (guessed) #color string is not valid, entered color will
    # be inserted at the current cursor position.
    if not color.isValid():
        color = QtGui.QColor(kate.configuration[_INSERT_COLOR_LCC])
        if not view.selection():
            colorRange = KTextEditor.Range(cursor, cursor)  # Will not override the text under cursor...
    # Choose a #color via dialog
    result = kdeui.KColorDialog.getColor(color)
    if result == kdeui.KColorDialog.Accepted:               # Is user pressed Ok?
        colorStr = color.name()                             # Get it as #color text
        # Remember for future use
        kate.configuration[_INSERT_COLOR_LCC] = colorStr
        document.startEditing()
        document.replaceText(colorRange, colorStr)          # Replace selected/found range w/ a new text
        document.endEditing()
        # Select just entered #color, if smth was selected before
        if view.selection():
            startPos = colorRange.start()
            endPos = KTextEditor.Cursor(startPos.line(), startPos.column() + len(colorStr))
            view.setSelection(KTextEditor.Range(startPos, endPos))


@kate.init
def init():
    # Set default value for last used #color if not configured yet
    if "lastUsedColor" not in kate.configuration:
        kate.configuration[_INSERT_COLOR_LCC] = 'white'

# kate: indent-width 4;
