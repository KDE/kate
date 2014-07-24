# -*- coding: utf-8 -*-
#
# Kate/Pâté plugins to work with code blocks
# Copyright 2010-2013 by Alex Turbov <i.zaufi@gmail.com>
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

'''Plugins to help code editing'''

from PyKDE4.kdecore import i18nc
from PyKDE4.ktexteditor import KTextEditor

import kate

from libkatepate import common


@kate.action
def insertCharFromLineAbove():
    '''Add the same char to the current cursor position as at the line above'''
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


@kate.action
def insertCharFromLineBelow():
    '''Add the same char to the current cursor position as at the line below'''
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


@kate.action
def killRestOfLine():
    '''Remove text from cursor position to the end of the current line'''
    doc = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()

    endPosition = KTextEditor.Cursor(pos.line(), doc.lineLength(pos.line()))
    killRange = KTextEditor.Range(pos, endPosition)

    doc.startEditing()
    doc.removeText(killRange)
    doc.endEditing()


@kate.action
def killLeadOfLine():
    ''' Remove text from a start of a line to the current cursor position
        but keep leading spaces (to avoid breaking indentation)

        NOTE This function suppose spaces as indentation character!
        TODO Get indent character from config
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


def _wrapRange(rangeToWrap, openCh, closeCh, doc = None):
    if not doc:
        doc = kate.activeDocument()

    doc.startEditing()                                      # Start atomic UnDo operation
    doc.replaceText(rangeToWrap, openCh + doc.text(rangeToWrap) + closeCh)
    doc.endEditing()                                        # Done editing


def _wrapBlockWithChar(openCh, closeCh, indentMultiline = True):
    '''Wrap a current word or selection (if any) into given open and close chars

       If current selection is multiline, add one indentation level and put
       open/close chars on separate lines
    '''
    doc = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()

    selectedRange = view.selectionRange()
    if selectedRange.isEmpty():
        # No text selected. Ok, lets wrap a word where cursor positioned
        wordRange = common.getBoundTextRangeSL(
            common.CXX_IDENTIFIER_BOUNDARIES
          , common.CXX_IDENTIFIER_BOUNDARIES
          , pos
          , doc
          )
        _wrapRange(wordRange, openCh, closeCh, doc)
    else:
        if selectedRange.start().line() == selectedRange.end().line() or indentMultiline == False:
            # single line selection (or no special indentation required)
            _wrapRange(selectedRange, openCh, closeCh, doc)

            # extend current selection
            selectedRange = KTextEditor.Range(
                selectedRange.start()
              , KTextEditor.Cursor(
                    selectedRange.end().line()
                  , selectedRange.end().column() + len(openCh) + len(closeCh)
                  )
              )
            view.setSelection(selectedRange)
        else:
            # Try to extend selection to be started from 0 columns at both ends
            common.extendSelectionToWholeLine(view)
            selectedRange = view.selectionRange()

            # multiline selection
            # 0) extend selection to capture whole lines
            gap = ' ' * common.getLineIndentation(selectedRange.start().line(), doc)
            # TODO Get indent width (and char) from config (get rid of hardcocded '4')
            text = gap + openCh + '\n' \
              + '\n'.join([' ' * 4 + line for line in doc.text(selectedRange).split('\n')[:-1]]) \
              + '\n' + gap + closeCh + '\n'
            doc.startEditing()
            doc.replaceText(selectedRange, text)
            doc.endEditing()

            # extend current selection
            r = KTextEditor.Range(selectedRange.start().line(), 0, selectedRange.end().line() + 2, 0)
            view.setSelection(r)


@kate.action
def wrapBlockWithRoundBrackets():
    '''Wrap current word (identifier) or selection into pair of '(' and ')' characters'''
    _wrapBlockWithChar('(', ')')

@kate.action
def wrapBlockWithSquareBrackets():
    '''Wrap current word (identifier) or selection into pair of '[' and ']' characters'''
    _wrapBlockWithChar('[', ']')

@kate.action
def wrapBlockWithCurveBrackets():
    '''Wrap current word (identifier) or selection into pair of '{' and '}' characters'''
    _wrapBlockWithChar('{', '}')

@kate.action
def wrapBlockWithAngleBrackets():
    '''Wrap current word (identifier) or selection into pair of '<' and '>' characters'''
    _wrapBlockWithChar('<', '>')

@kate.action
def wrapBlockWithDblQuotes():
    '''Wrap current word (identifier) or selection into pair of '"' characters'''
    _wrapBlockWithChar('"', '"', False)

@kate.action
def wrapBlockWithQuotes():
    '''Wrap current word (identifier) or selection into pair of '"' characters'''
    _wrapBlockWithChar("'", "'", False)

# kate: indent-width 4;
