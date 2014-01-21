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

import sys

import kate
import kate.view

from .color_chooser import ColorChooser
from .color_palette import PaletteView
from .color_swatcher import ColorSwatcher


paletteView = None
colorChooserWidget = None
swatcher = ColorSwatcher()


@kate.action
def insertColor():
    '''Insert/edit color string using color chooser dialog

    If cursor positioned in a color string, this action will edit it, otherwise
    a new color string will be inserted into a document.
    '''
    document = kate.activeDocument()
    view = kate.activeView()

    global colorChooserWidget
    assert colorChooserWidget is not None
    colorChooserWidget.updateColors(document)
    colorChooserWidget.moveAround(view.cursorPositionCoordinates())


@kate.viewChanged
@kate.viewCreated
def viewChanged(view=None):
    ''' Rescan current document on view create and/or change'''
    global paletteView
    if paletteView is not None:
        paletteView.update(view)


@kate.view.selectionChanged
def selectionChanged(view):
    global swatcher
    if swatcher is not None and view is not None:
        swatcher.show_swatch(view)


@kate.init
def init():
    '''Iniialize global variables and read config'''

    # Make an instance of a palette tool view
    global paletteView
    assert paletteView is None
    paletteView = PaletteView()

    # Make an instance of a color chooser widget,
    # connect it to active document updater
    global colorChooserWidget
    assert colorChooserWidget is None
    colorChooserWidget = ColorChooser()


@kate.unload
def destroy():
    global paletteView
    assert paletteView is not None
    del paletteView
    paletteView = None

    global colorChooserWidget
    assert colorChooserWidget is not None
    del colorChooserWidget
    colorChooserWidget = None

    global swatcher
    assert swatcher is not None
    del swatcher
    swatcher = None

    del sys.modules['color_tools.color_palette']
    del sys.modules['color_tools.color_chooser']
    del sys.modules['color_tools.color_swatcher']
    del sys.modules['color_tools.utils']
