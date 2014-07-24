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
from libkatepate.selection import setSelection

TEXT_TO_CHANGE = '${cursor}'


def checkValidIndent(indent):
    for char in indent:
        if char not in ('\t', ''):
            return False
    return True


def convertToValidIndent(indent, doc):
    if not checkValidIndent(indent):
        try:
            ci = doc.configInterface()
            if ci.configValue("replace-tabs"):
                return ' ' * len(indent)
            chars = 0
            tabs = 0
            last_pos_char = -1
            for i, char in enumerate(indent):
                if not char in (' ', '\t'):
                    last_pos_char = i
            for char in indent[:last_pos_char + 1]:
                if char == '\t':
                    tabs += 1
                else:
                    chars += 1
            width = ci.configValue('tab-width')
            tabs += chars / width
            chars = chars % width
            valid_indent = '\t' * tabs + ' ' * chars + indent[last_pos_char + 1:]
            return valid_indent.replace(' ' * width, '\t')
        except AttributeError:
            return ' ' * len(indent)
    return indent


def insertText(text, strip_line=False,
               start_in_current_column=False,
               select_text=True):
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    currentPosition = view.cursorPosition()
    if strip_line:
        text = '\n'.join([line.strip() for line in text.splitlines()])
    if start_in_current_column:
        line = currentPosition.line()
        column = currentPosition.column()
        ident = convertToValidIndent(currentDocument.line(line)[:column], currentDocument)
        text = '\n'.join([i > 0 and '%s%s' % (ident, l) or l
                          for i, l in enumerate(text.splitlines())])
    currentDocument.insertText(currentPosition, text)
    if select_text and TEXT_TO_CHANGE in text:
        text_to_change_len = len(TEXT_TO_CHANGE)
        pos_cursor = text.index(TEXT_TO_CHANGE)
        line = text[:pos_cursor].count("\n") + currentPosition.line()
        text_line = currentDocument.line(line)
        column = text_line.index(TEXT_TO_CHANGE)
        setSelection((line, column), (line, column + text_to_change_len))
