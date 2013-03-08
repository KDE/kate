#
# Kate/Pâté color plugins
# Copyright 2010-2013 by Alex Turbov <i.zaufi@gmail.com>
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

"""Shows a preview of a of a selected color string (like e.g. #fe57a1) in a tooltip \
and/or all parsed #colors in a 'Palette' tool view, ability to edit/insert #color strings.
"""

import os
import math
import string

from PyQt4 import uic
from PyQt4.QtCore import QEvent, QObject, QTimer, Qt, pyqtSlot
from PyQt4.QtGui import QColor, QPalette, QToolTip, QWidget

from PyKDE4.kdecore import i18n
from PyKDE4.kdeui import KColorDialog, KColorCells
from PyKDE4.ktexteditor import KTextEditor

import kate

from libkatepate import common


_INSERT_COLOR_LCC = 'insertColor:lastUsedColor'
_CELLS_COUNT_PER_ROW = 5
paletteView = None

@kate.action('Insert Color', shortcut='Meta+Shift+C', icon='color', menu='Tools')
def insertColor():
    """Insert/edit color string using color chooser dialog

    If cursor positioned in a color string, this action will edit it, otherwise
    a new color string will be inserted into a document.
    """
    document = kate.activeDocument()
    view = kate.activeView()
    cursor = view.cursorPosition()

    if view.selection(): # Some text selected, just use it as input...
        color_range = view.selectionRange()
    else:                                                   # If no selection, try to get a #color under cursor
        color_range = common.getBoundTextRangeSL(
            common.IDENTIFIER_BOUNDARIES - {'#'}
          , common.IDENTIFIER_BOUNDARIES
          , cursor
          , document
          )

    if color_range.isValid():
        current_color = document.text(color_range)
    else:
        current_color = kate.configuration[_INSERT_COLOR_LCC]

    color = QColor(current_color)
    # If no text selected (i.e. user doesn’t want to override selection)
    # and (guessed) color string is not valid, entered color will
    # be inserted at the current cursor position.
    if not color.isValid():
        color = QColor(kate.configuration[_INSERT_COLOR_LCC])
        if not view.selection():
            color_range = KTextEditor.Range(cursor, 0)      # Will not override the text under cursor…
    # Choose a color via dialog
    result = KColorDialog.getColor(color)
    if result == KColorDialog.Accepted:                     # Did user press OK?
        color_str = color.name()                            # Get it as color string
        # Remember for future use
        kate.configuration[_INSERT_COLOR_LCC] = color_str
        document.startEditing()
        document.replaceText(color_range, color_str)        # Replace selected/found range w/ a new text
        document.endEditing()
        # Select just entered #color, if something was selected before
        if view.selection():
            start_pos = color_range.start()
            view.setSelection(KTextEditor.Range(start_pos, len(color_str)))


class ColorSwatcher:
    """Class encapsuling the ability to show color swatches (colored tooltips)"""
    swatch_template = '<div>{}</div>'.format('&nbsp;' * 10)

    def __init__(self):
        self.old_palette = None
        kate.viewChanged(self.view_changed)
        self.view_changed()

    def view_changed(self):
        """Connects a swatch showing slot to each view’s selection change signal"""
        view = kate.activeView()
        if view:
            view.selectionChanged.connect(self.show_swatch)

    def show_swatch(self, view):
        """Shows the swatch if a valid color is selected"""
        if view.selection():
            color = QColor(view.selectionText())
            if color.isValid():
                cursor_pos = view.cursorPositionCoordinates()
                QToolTip.showText(cursor_pos, self.swatch_template)
                self.change_palette(color)

    def change_palette(self, color):
        """Sets the global tooltip background to the given color and initializes reset"""
        self.old_palette = QToolTip.palette()
        p = QPalette(self.old_palette)
        p.setColor(QPalette.All, QPalette.ToolTipBase, color)
        QToolTip.setPalette(p)

        self.timer = QTimer()
        self.timer.timeout.connect(self.try_reset_palette)
        self.timer.start(300) #short enough not to flicker a wrongly colored tooltip

    def try_reset_palette(self):
        """Resets the global tooltip background color as soon as the swatch is hidden"""
        if self.old_palette is not None and not QToolTip.isVisible():
            QToolTip.setPalette(self.old_palette)
            self.old_palette = None


class ColorRangePair:
    """Simple class to store a #color associated w/ a location in a document"""
    color = None
    color_range = None

    def __init__(self, color, color_range):
        self.color = color
        self.color_range = color_range


class PaletteView(QObject):
    """A toolvide to display palette of the current document"""
    colors = []
    toolView = None
    colorCellsWidget = None

    def __init__(self, parent):
        super(PaletteView, self).__init__(parent)
        self.toolView = kate.mainInterfaceWindow().createToolView(
            "color_tools_pate_plugin"
          , kate.Kate.MainWindow.Bottom
          , kate.gui.loadIcon('color')
          , i18n("Palette")
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
        self.colorCellsWidget.setAcceptDrags(False)
        interior.verticalLayout.addWidget(self.colorCellsWidget)
        self.colorCellsWidget.colorSelected.connect(self.colorSelected)
        self.colorCellsWidget.colorDoubleClicked.connect(self.colorDoubleClicked)

    def __del__(self):
        """Plugins that use a toolview need to delete it for reloading to work."""
        if self.toolView:
            self.toolView.deleteLater()
            self.toolView = None

    def eventFilter(self, obj, event):
        """Hide the Palette tool view on ESCAPE key"""
        if event.type() == QEvent.KeyPress and event.key() == Qt.Key_Escape:
            kate.mainInterfaceWindow().hideToolView(self.toolView)
            return True
        return self.toolView.eventFilter(obj, event)

    def updateColors(self, view=None):
        """Scan a document for #colors"""
        self.colors = list()                                # Clear previous colors
        if view:
            document = view.document()
        else:
            document = kate.activeDocument()
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
                if color.isValid():
                    self.colors.append(ColorRangePair(color, color_range))
                    print('PALETTE VIEW: Found %s' % color_str)
                start = end

    def updateColorCells(self):
        """Calculate rows*columns and fill the cells w/ #colors"""
        if len(self.colors):
            # Recalculate rows/columns
            columns = int(math.sqrt(len(self.colors)))
            rows = int(len(self.colors) / columns) + int(bool(len(self.colors) % columns))
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

    @pyqtSlot()
    def update(self, view=None):
        self.updateColors(view)
        self.updateColorCells()

    @pyqtSlot(int, QColor)
    def colorSelected(self, idx, color):
        """Move cursor to the position of the selected #color and select the range"""
        view = kate.activeView()
        view.setCursorPosition(self.colors[idx].color_range.start())
        view.setSelection(self.colors[idx].color_range)

    @pyqtSlot(int, QColor)
    def colorDoubleClicked(self, idx, color):
        """Edit selected color on double click"""
        insertColor()
        self.update()


@kate.viewChanged
@kate.viewCreated
def viewChanged(view=None):
    """ Rescan current document on view create and/or change"""
    global paletteView
    return paletteView.update(view)


@kate.init
def init():
    """Iniialize global variables and read config"""
    # Set default value for last used #color if not configured yet
    if _INSERT_COLOR_LCC not in kate.configuration:
        kate.configuration[_INSERT_COLOR_LCC] = 'white'

    swatcher = ColorSwatcher()

    # Make an instance of a palette tool view
    global paletteView
    if paletteView is None:
        paletteView = PaletteView(kate.mainWindow())

@kate.unload
def destroy():
    """Plugins that use a toolview need to delete it for reloading to work."""
    global paletteView
    if paletteView:
        paletteView.__del__()
        paletteView = None

# kate: space-indent on; indent-width 4;
