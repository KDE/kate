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

import math
import string

from PyQt4.QtCore import QEvent, Qt, pyqtSlot, pyqtSignal
from PyQt4.QtGui import QColor, QFrame, QVBoxLayout

from PyKDE4.kdecore import i18nc
from PyKDE4.kdeui import KColorDialog, KColorCells, KPushButton
from PyKDE4.ktexteditor import KTextEditor

import kate

from libkatepate import common

from .utils import setTooltips



class ColorChooser(QFrame):
    '''
        Completion-like widget to quick select hexadecimal colors used in a document

        TODO Make cell/widget size configurable?
    '''

    _INSERT_COLOR_LCC = 'insertColor:lastUsedColor'

    colorSelected = pyqtSignal(QColor)

    def __init__(self):
        super(ColorChooser, self).__init__(None)
        self.colors = KColorCells(self, 1, 1)
        self.colors.setAcceptDrags(False)
        self.colors.setEditTriggers(self.colors.NoEditTriggers)
        self.otherBtn = KPushButton(self)
        self.otherBtn.setText(i18nc('@action:button', '&Other...'))
        layout = QVBoxLayout(self)
        layout.addWidget(self.colors)
        layout.addWidget(self.otherBtn)
        self.setFocusPolicy(Qt.StrongFocus)
        self.setFrameShape(QFrame.Panel)
        self.setWindowFlags(Qt.FramelessWindowHint | Qt.Popup)

        # Set default value for last used #color if not configured yet
        if ColorChooser._INSERT_COLOR_LCC not in kate.sessionConfiguration:
            kate.sessionConfiguration[ColorChooser._INSERT_COLOR_LCC] = '#ffffff'

        # Subscribe to observe widget events
        # - open KColorDialog on 'Other...' button click
        self.otherBtn.clicked.connect(self._show_color_dialog)
        # - select color by RMB click or ENTER on keyboard
        self.colors.cellActivated.connect(self._color_selected)
        # -
        self.colorSelected.connect(self._insert_color_into_active_document)

        self.installEventFilter(self)


    def __del__(self):
        mw = kate.mainInterfaceWindow()
        if mw:
            self.blockSignals(True)
            self.removeEventFilter(self)


    def updateColors(self, document):
        '''Scan a given document and collect unique colors

            Returns a list of QColor objects.
        '''
        result = []
        # Iterate over document's lines trying to find #colors
        for l in range(0, document.lines()):
            line = document.line(l)                         # Get the current line
            start = 0                                       # Set initial position to 0 (line start)
            while start < len(line):                        # Repeat 'till the line end
                start = line.find('#', start)               # Try to find a '#' character (start of #color)
                if start == -1:                             # Did we found smth?
                    break                                   # No! Nothing to do...
                # Try to get a word right after the '#' char
                end = start + 1
                for c in line[end:]:
                    if not (c in string.hexdigits or c in string.ascii_letters):
                        break
                    end += 1
                color_range = KTextEditor.Range(l, start, l, end)
                color_str = document.text(color_range)
                color = QColor(color_str)
                if color.isValid() and color not in result:
                    result.append(color)
                    kate.kDebug('ColorUtils: scan for #colors found {}'.format(color_str))
                start = end
        self._set_colors(result)


    def moveAround(self, position):
        '''Smart positioning self around cursor'''
        self.colors.resizeColumnsToContents()
        self.colors.resizeRowsToContents()
        mwg = kate.mainWindow().geometry()
        wc = mwg.center()
        pos = mwg.topLeft() + position
        pos.setY(pos.y() + 40)
        if wc.y() < pos.y():
            pos.setY(pos.y() - self.height())
        if wc.x() < pos.x():
            pos.setX(pos.x() - self.width())

        self.move(pos)


    def emitSelectedColorHideSelf(self, color):
        # Remember last selected color for future preselect
        kate.sessionConfiguration[ColorChooser._INSERT_COLOR_LCC] = color.name()
        self.colorSelected.emit(color)
        self.hide()


    def eventFilter(self, obj, event):
        '''Hide self on Esc key'''
        if event.type() == QEvent.KeyPress and event.key() == Qt.Key_Escape:
            self.hide()
            return True
        return super(ColorChooser, self).eventFilter(obj, event)


    def _set_colors(self, colors):
        if len(colors):
            rows, columns = ColorChooser._calc_dimensions_for_items_count(len(colors))
        else:
            self.hide()
            self._show_color_dialog(True)
            return
        self.colors.setColumnCount(columns)
        self.colors.setRowCount(rows)
        self.colors.setMinimumSize(20 * columns, 25 * rows)
        self.updateGeometry()
        self.adjustSize()
        self.colors.resizeColumnsToContents()
        self.colors.resizeRowsToContents()
        # Fill color cells
        for i, color in enumerate(colors):
            self.colors.setColor(i, color)
        setTooltips(rows, columns, self.colors)             # Set tooltips for all valid color cells
        self.colors.setFocus()                              # Give a focus to widget
        self.show()                                         # Show it!


    @pyqtSlot(bool)
    def _show_color_dialog(self, f):
        '''Get color using KColorDialog'''
        # Preselect last used color
        color = QColor(kate.sessionConfiguration[ColorChooser._INSERT_COLOR_LCC])
        result = KColorDialog.getColor(color)
        if result == KColorDialog.Accepted:                 # Did user press OK?
            self.emitSelectedColorHideSelf(color)


    @pyqtSlot(int, int)
    def _color_selected(self, row, column):
        '''Smth has selected in KColorCells'''
        color = self.colors.item(row, column).data(Qt.BackgroundRole)
        #kate.kDebug('ColorUtils: activated row={}, column={}, color={}'.format(row, column, color))
        self.emitSelectedColorHideSelf(color)


    @pyqtSlot(QColor)
    def _insert_color_into_active_document(self, color):
        color_str = color.name()                            # Get it as color string
        #kate.kDebug('ColorUtils: selected color: {}'.format(color_str))
        document = kate.activeDocument()
        view = kate.activeView()
        has_selection = view.selection()                    # Remember current selection state
        color_range = ColorChooser._get_color_range_under_cursor(view)
        document.startEditing()
        document.replaceText(color_range, color_str)        # Replace selected/found range w/ a new text
        document.endEditing()
        # Select just entered #color, if something was selected before
        if has_selection:
            start_pos = color_range.start()
            view.setSelection(KTextEditor.Range(start_pos, len(color_str)))


    @staticmethod
    def _calc_dimensions_for_items_count(count):
        '''Recalculate rows/columns of table view for given items count'''
        rows = int(math.sqrt(count))
        columns = int(count / rows) + int(bool(count % rows))
        return (rows, columns)


    @staticmethod
    def _get_color_range_under_cursor(view):
        assert(view is not None)
        if view.selection():                                # Some text selected, just use it as input...
            color_range = view.selectionRange()
        else:                                               # If no selection, try to get a #color under cursor
            color_range = common.getBoundTextRangeSL(
                common.IDENTIFIER_BOUNDARIES - {'#'}
              , common.IDENTIFIER_BOUNDARIES
              , view.cursorPosition()
              , view.document()
              )
            # Check if a word under cursor is a valid #color
            color = QColor(view.document().text(color_range))
            if not color.isValid():
                color_range = KTextEditor.Range(view.cursorPosition(), view.cursorPosition())
        return color_range
