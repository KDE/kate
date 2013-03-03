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

"""Shows a preview of a of a selected color string (like e.g. #fe57a1) \
and adds a menu items to insert such color stings
"""

import kate
from kate.gui import QColor, QToolTip, QPalette, QTimer

from PyKDE4.kdeui import KColorDialog
from PyKDE4.ktexteditor import KTextEditor

from libkatepate import common


_INSERT_COLOR_LCC = 'insertColor:lastUsedColor'


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
            color_range = KTextEditor.Range(cursor, 0)  # Will not override the text under cursor…
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


@kate.init
def _init():
    # Set default value for last used #color if not configured yet
    if _INSERT_COLOR_LCC not in kate.configuration:
        kate.configuration[_INSERT_COLOR_LCC] = 'white'

    swatcher = ColorSwatcher()

# kate: space-indent on; mixedindent off; indent-width 4;
