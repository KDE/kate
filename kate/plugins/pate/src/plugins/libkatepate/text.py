# -*- coding: utf-8 -*-
#
# Copyright 2013 by Pablo Martín <goinnn@gmail.com>
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
import kate
from libkatepate.selection import setSelectionFromCurrentPosition

TEXT_TO_CHANGE = '${cursor}'


def insertText(text, strip_line=False,
               start_in_current_column=False,
               delete_spaces_initial=False,
               move_to=True):
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    currentPosition = view.cursorPosition()
    spaces = ''
    if strip_line:
        text = '\n'.join([line.strip() for line in text.splitlines()])
    if start_in_current_column:
        number_of_spaces = currentPosition.position()[1]
        spaces = ' ' * number_of_spaces
        text = '\n'.join([i > 0 and '%s%s' % (spaces, line) or line
                            for i, line in enumerate(text.splitlines())])
    if delete_spaces_initial:
        currentPosition.setColumn(0)
    currentDocument.insertText(currentPosition, text)
    text_to_change_len = len(TEXT_TO_CHANGE)
    if move_to and TEXT_TO_CHANGE in text:
        currentPosition = view.cursorPosition()
        pos_cursor = text.index(TEXT_TO_CHANGE)
        lines = text[pos_cursor + text_to_change_len:].count('\n')
        column = len(text[:pos_cursor].split('\n')[-1]) - currentPosition.column()
        setSelectionFromCurrentPosition((-lines, column), (-lines, column + text_to_change_len))
