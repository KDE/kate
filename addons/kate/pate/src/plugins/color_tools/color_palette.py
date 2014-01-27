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

import os
import string

from PyQt4 import uic
from PyQt4.QtCore import QEvent, QObject, Qt, pyqtSlot
from PyQt4.QtGui import QColor, QWidget

from PyKDE4.kdecore import i18nc
from PyKDE4.kdeui import KColorCells, KIcon
from PyKDE4.ktexteditor import KTextEditor

import kate

from .utils import setTooltips


class ColorRangePair:
    '''Simple class to store a #color associated w/ a location in a document'''
    color = None
    color_range = None

    def __init__(self, color, color_range):
        self.color = color
        self.color_range = color_range


class PaletteView(QObject):
    '''A toolview to display palette of the current document'''
    colors = []
    toolView = None
    colorCellsWidget = None

    def __init__(self):
        super(PaletteView, self).__init__(None)
        self.toolView = kate.mainInterfaceWindow().createToolView(
            'color_tools_pate_plugin'
          , kate.Kate.MainWindow.Bottom
          , KIcon('color').pixmap(32, 32)
          , i18nc('@title:tab', 'Palette')
          )
        self.toolView.installEventFilter(self)
        # By default, the toolview has box layout, which is not easy to delete.
        # For now, just add an extra widget.
        top = QWidget(self.toolView)
        # Set up the user interface from Designer.
        interior = uic.loadUi(os.path.join(os.path.dirname(__file__), 'color_tools_toolview.ui'), top)
        interior.update.clicked.connect(self.update)
        self.colorCellsWidget = KColorCells(interior, 1, 1)
        # TODO Don't know how to deal w/ drag-n-drops :(
        # It seems there is no signal to realize that some item has changed :(
        # (at lieast I didn't find it)
        self.colorCellsWidget.setAcceptDrags(False)
        self.colorCellsWidget.setEditTriggers(self.colorCellsWidget.NoEditTriggers)
        interior.verticalLayout.addWidget(self.colorCellsWidget)
        self.colorCellsWidget.colorSelected.connect(self.colorSelected)
        self.colorCellsWidget.colorDoubleClicked.connect(self.colorDoubleClicked)


    def __del__(self):
        '''Plugins that use a toolview need to delete it for reloading to work.'''
        assert(self.toolView is not None)
        mw = kate.mainInterfaceWindow()
        if mw:
            mw.hideToolView(self.toolView)
            mw.destroyToolView(self.toolView)
            self.toolView.removeEventFilter(self)
        self.colorCellsWidget = None
        self.toolView = None


    def eventFilter(self, obj, event):
        '''Hide the Palette tool view on ESCAPE key'''
        if event.type() == QEvent.KeyPress and event.key() == Qt.Key_Escape:
            kate.mainInterfaceWindow().hideToolView(self.toolView)
            return True
        return self.toolView.eventFilter(obj, event)


    def updateColors(self, view=None):
        '''Scan a document for #colors

            Returns a list of tuples: QColor and range in a document
            TODO Some refactoring needed to reduce code duplication
            (@sa _get_color_range_under_cursor())
        '''
        self.colors = []                                    # Clear previous colors
        if view:
            document = view.document()
        else:
            try:
                document = kate.activeDocument()
            except kate.NoActiveView:
                return                                      # Do nothing if we can't get a current document
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
                    if not (c in string.hexdigits):
                        break
                    end += 1
                color_range = KTextEditor.Range(l, start, l, end)
                color_str = document.text(color_range)
                color = QColor(color_str)
                if color.isValid():
                    self.colors.append(ColorRangePair(color, color_range))
                    kate.kDebug('ColorUtilsToolView: scan for #colors found {}'.format(color_str))
                start = end


    def updateColorCells(self):
        '''Calculate rows*columns and fill the cells w/ #colors'''
        if len(self.colors):
            # Recalculate rows/columns
            columns = int(self.colorCellsWidget.width() / 30)
            visible_rows = int(self.colorCellsWidget.height() / 25)
            if len(self.colors) < (columns * visible_rows):
                rows = visible_rows
            else:
                visible_cells = columns * visible_rows
                rest = len(self.colors) - visible_cells
                rows = visible_rows + int(rest / columns) + int(bool(rest % columns))
        else:
            columns = 1
            rows = 1
            self.colors.append(ColorRangePair(QColor(), KTextEditor.Range()))
        self.colorCellsWidget.setColumnCount(columns)
        self.colorCellsWidget.setRowCount(rows)
        self.colorCellsWidget.resizeColumnsToContents()
        self.colorCellsWidget.resizeRowsToContents()
        # Fill color cells
        for i, crp in enumerate(self.colors):
            self.colorCellsWidget.setColor(i, crp.color)
        for i in range(len(self.colors), columns * rows):
            self.colorCellsWidget.setColor(i, QColor())
        setTooltips(rows, columns, self.colorCellsWidget)


    @pyqtSlot()
    def update(self, view=None):
        self.updateColors(view)
        self.updateColorCells()


    @pyqtSlot(int, QColor)
    def colorSelected(self, idx, color):
        '''Move cursor to the position of the selected #color and select the range'''
        view = kate.activeView()
        view.setCursorPosition(self.colors[idx].color_range.start())
        view.setSelection(self.colors[idx].color_range)


    @pyqtSlot(int, QColor)
    def colorDoubleClicked(self, idx, color):
        '''Edit selected color on double click'''
        insertColor()
        self.update()
