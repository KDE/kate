# -*- coding: utf-8 -*-
#
# Kate/Pâté plugins to work with code blocks
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
#   Insert Char From Line Above (Meta+E)
#       add the same char to the current cursor position as in the line above
#
#   Insert Char From Line Below (Meta+W)
#       add the same char to the current cursor position as in the line below
#
#   Kill Text After Cursor (Meta+K)
#       remove text from cursor position to the end of the current line
#
#   Kill Text Before Cursor (Meta+U)
#       remove text from cursor position to the start of the current line
#       but keep leading spaces (to avoid breaking indentation)
#

import kate
import kate.gui

from PyKDE4.ktexteditor import KTextEditor

from libkatepate import ui, common

@kate.action('Insert Char From Line Above', shortcut='Meta+E')
def insertCharFromLineAbove():
    doc = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()

    if pos.line() == 0:
        return

    lineAbove = doc.line(pos.line() - 1)
    char = lineAbove[pos.column():pos.column() + 1]
    if not bool(char):
        return

    doc.startEditing()
    doc.insertText(pos, char)
    doc.endEditing()


@kate.action('Insert Char From Line Below', shortcut='Meta+W')
def insertCharFromLineBelow():
    doc = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()

    if pos.line() == 0:
        return

    lineBelow = doc.line(pos.line() + 1)
    char = lineBelow[pos.column():pos.column() + 1]
    if not bool(char):
        return

    doc.startEditing()
    doc.insertText(pos, char)
    doc.endEditing()


@kate.action('Kill Text After Cursor', shortcut='Meta+K')
def killRestOfLine():
    doc = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()

    endPosition = KTextEditor.Cursor(pos.line(), doc.lineLength(pos.line()))
    killRange = KTextEditor.Range(pos, endPosition)

    doc.startEditing()
    doc.removeText(killRange)
    doc.endEditing()


@kate.action('Kill Text Before Cursor', shortcut='Meta+U')
def killLeadOfLine():
    ''' Remove text from a start of a line to the current crsor position

        NOTE This function suppose spaces as indentation character!
    '''
    doc = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()

    indent = common.getCurrentLineIndentation(view)
    startPosition = KTextEditor.Cursor(pos.line(), 0)
    killRange = KTextEditor.Range(startPosition, pos)

    doc.startEditing()
    doc.removeText(killRange)
    doc.insertText(startPosition, ' ' * indent)
    doc.endEditing()
