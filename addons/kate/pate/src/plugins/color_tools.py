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
# Here is a short list of plugins in this file:
#
#   Insert Color (Meta+Shift+C)
#       open color choose dialog and insert selected color as #hex string
#
#
#   Color Swatches
#       Select a color string of any kind to display its color in a tooltip
#

'''Utilities to work with hexadecimal colors in documents

    Shows a preview of a selected hexadecimal color string (e.g. #fe57a1) in a tooltip
    and/or all parsed hexadecimal colors in a 'Palette' tool view, ability to edit/insert 
    hexadecimal color strings.

'''

import os
import math
import string

from PyQt4 import uic
from PyQt4.QtCore import QEvent, QObject, QTimer, Qt, pyqtSlot, pyqtSignal
from PyQt4.QtGui import QColor, QFrame, QPalette, QToolTip, QWidget, QVBoxLayout

from PyKDE4.kdecore import i18nc
from PyKDE4.kdeui import KColorDialog, KColorCells, KPushButton, KIcon
from PyKDE4.ktexteditor import KTextEditor

import kate

from libkatepate import common


_INSERT_COLOR_LCC = 'insertColor:lastUsedColor'
_CELLS_COUNT_PER_ROW = 5

paletteView = None
colorChooserWidget = None


def _calc_dimensions_for_items_count(count):
    '''Recalculate rows/columns of table view for given items count'''
    rows = int(math.sqrt(count))
    columns = int(count / rows) + int(bool(count % rows))
    return (rows, columns)


def _set_tooltips(rows, columns, colors):
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


class ColorChooser(QFrame):
    '''
        Completion-like widget to quick select hexadecimal colors used in a document

        TODO Make cell/widget size configurable?
    '''
    colorSelected = pyqtSignal(QColor)

    def __init__(self, parent):
        super(ColorChooser, self).__init__(parent)
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
        self.setWindowFlags(Qt.FramelessWindowHint | Qt.Popup);

        # Subscribe to observe widget events
        # - open KColorDialog on 'Other...' button click
        self.otherBtn.clicked.connect(self.show_color_dialog)
        # - select color by RMB click or ENTER on keyboard
        self.colors.cellActivated.connect(self.color_selected)

        self.installEventFilter(self)


    def setColors(self, colors):
        if len(colors):
            rows, columns = _calc_dimensions_for_items_count(len(colors))
        else:
            self.hide()
            self.show_color_dialog(True)
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
        _set_tooltips(rows, columns, self.colors)           # Set tooltips for all valid color cells
        self.colors.setFocus()                              # Give a focus to widget
        self.show()                                         # Show it!


    @pyqtSlot(bool)
    def show_color_dialog(self, f):
        '''Get color using KColorDialog'''
        # Preselect last used color
        color = QColor(kate.sessionConfiguration[_INSERT_COLOR_LCC])
        result = KColorDialog.getColor(color)
        if result == KColorDialog.Accepted:                  # Did user press OK?
            self.emitSelectedColorHideSelf(color)


    @pyqtSlot(int, int)
    def color_selected(self, row, column):
        '''Smth has selected in KColorCells'''
        color = self.colors.item(row, column).data(Qt.BackgroundRole)
        #kate.qDebug('ColorUtils: activated row={}, column={}, color={}'.format(row, column, color))
        self.emitSelectedColorHideSelf(color)


    def emitSelectedColorHideSelf(self, color):
        # Remember last selected color for future preselect
        kate.sessionConfiguration[_INSERT_COLOR_LCC] = color.name()
        self.colorSelected.emit(color)
        self.hide()


    def eventFilter(self, obj, event):
        '''Hide self on Esc key'''
        if event.type() == QEvent.KeyPress and event.key() == Qt.Key_Escape:
            self.hide()
            return True
        return super(ColorChooser, self).eventFilter(obj, event)


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


def _collect_colors(document):
    '''Scan a given document and collect unique colors

        Returns a list of QColor objects.
    '''
    result = []
    # Iterate over document's lines trying to find #colors
    for l in range(0, document.lines()):
        line = document.line(l)                             # Get the current line
        start = 0                                           # Set initial position to 0 (line start)
        while start < len(line):                            # Repeat 'till the line end
            start = line.find('#', start)                   # Try to find a '#' character (start of #color)
            if start == -1:                                 # Did we found smth?
                break                                       # No! Nothing to do...
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
                kate.qDebug('ColorUtils: scan for #colors found {}'.format(color_str))
            start = end
    return result


def _get_color_range_under_cursor(view):
    assert(view is not None)
    if view.selection():                                    # Some text selected, just use it as input...
        color_range = view.selectionRange()
    else:                                                   # If no selection, try to get a #color under cursor
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


@kate.action
def insertColor():
    '''Insert/edit color string using color chooser dialog

    If cursor positioned in a color string, this action will edit it, otherwise
    a new color string will be inserted into a document.
    '''
    document = kate.activeDocument()
    view = kate.activeView()

    global colorChooserWidget
    colorChooserWidget.setColors(_collect_colors(document))
    colorChooserWidget.moveAround(view.cursorPositionCoordinates())


@pyqtSlot(QColor)
def _insertColorIntoActiveDocument(color):
    color_str = color.name()                                # Get it as color string
    #kate.qDebug('ColorUtils: selected color: {}'.format(color_str))
    document = kate.activeDocument()
    view = kate.activeView()
    cursor = view.cursorPosition()
    has_selection = view.selection()                        # Remember current selection state
    color_range = _get_color_range_under_cursor(view)
    document.startEditing()
    document.replaceText(color_range, color_str)            # Replace selected/found range w/ a new text
    document.endEditing()
    # Select just entered #color, if something was selected before
    if has_selection:
        start_pos = color_range.start()
        view.setSelection(KTextEditor.Range(start_pos, len(color_str)))


class ColorSwatcher:
    '''Class encapsuling the ability to show color swatches (colored tooltips)'''
    swatch_template = '<div>{}</div>'.format('&nbsp;' * 10)

    def __init__(self):
        self.old_palette = None
        kate.viewChanged(self.view_changed)
        self.view_changed()

    def view_changed(self):
        '''Connects a swatch showing slot to each view’s selection change signal'''
        view = kate.activeView()
        if view:
            view.selectionChanged.connect(self.show_swatch)

    def show_swatch(self, view):
        '''Shows the swatch if a valid color is selected'''
        if view.selection():
            color = QColor(view.selectionText())
            if color.isValid():
                cursor_pos = view.cursorPositionCoordinates()
                QToolTip.showText(cursor_pos, self.swatch_template)
                self.change_palette(color)

    def change_palette(self, color):
        '''Sets the global tooltip background to the given color and initializes reset'''
        self.old_palette = QToolTip.palette()
        p = QPalette(self.old_palette)
        p.setColor(QPalette.All, QPalette.ToolTipBase, color)
        QToolTip.setPalette(p)

        self.timer = QTimer()
        self.timer.timeout.connect(self.try_reset_palette)
        self.timer.start(300) #short enough not to flicker a wrongly colored tooltip

    def try_reset_palette(self):
        '''Resets the global tooltip background color as soon as the swatch is hidden'''
        if self.old_palette is not None and not QToolTip.isVisible():
            QToolTip.setPalette(self.old_palette)
            self.old_palette = None


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

    def __init__(self, parent):
        super(PaletteView, self).__init__(parent)
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

    #def __del__(self):
        #'''Plugins that use a toolview need to delete it for reloading to work.'''
        #if self.toolView:
            #self.toolView.deleteLater()
            #self.toolView = None

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
                    kate.qDebug('ColorUtilsToolView: scan for #colors found {}'.format(color_str))
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
        _set_tooltips(rows, columns, self.colorCellsWidget)


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


@kate.viewChanged
@kate.viewCreated
def viewChanged(view=None):
    ''' Rescan current document on view create and/or change'''
    global paletteView
    if paletteView is not None:
        return paletteView.update(view)


@kate.init
def init():
    '''Iniialize global variables and read config'''
    # Set default value for last used #color if not configured yet
    if _INSERT_COLOR_LCC not in kate.sessionConfiguration:
        kate.sessionConfiguration[_INSERT_COLOR_LCC] = '#ffffff'

    swatcher = ColorSwatcher()

    ## Make an instance of a palette tool view
    global paletteView
    if paletteView is None:
        paletteView = PaletteView(kate.mainWindow())

    # Make an instance of a color chooser widget,
    # connect it to active document updater
    global colorChooserWidget
    if colorChooserWidget is None:
        colorChooserWidget = ColorChooser(kate.mainWindow())
        colorChooserWidget.colorSelected.connect(_insertColorIntoActiveDocument)


@kate.unload
def destroy():
    '''Plugins that use a toolview need to delete it for reloading to work.'''
    global paletteView
    if paletteView:
        paletteView = None

    global colorChooserWidget
    if colorChooserWidget:
        colorChooserWidget = None


# kate: space-indent on; indent-width 4;
