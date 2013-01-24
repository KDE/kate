# -*- coding: utf-8 -*-
#
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

''' Reusable code for Kate/Pâté plugins: general purpose shared code '''

import kate

from PyKDE4.ktexteditor import KTextEditor

_COMMENT_STRINGS_MAP = {
    'Python'        : '# '
  , 'Perl'          : '# '
  , 'CMake'         : '# '
  , 'Bash'          : '# '
  , 'C++'           : '//'
  , 'C++11'         : '//'
  , 'C++11/Qt4'     : '//'
  , 'JavaScript'    : '//'
}

# NOTE ':' can be a part of full qualified name
CXX_IDENTIFIER_BOUNDARIES = set(' \t\n"\'[]{}()<>`~!@#$%^&*-+=|\\/?;,')

def isKnownCommentStyle(docType):
    ''' Check if we know how to comment a line in a given document type '''
    return docType in _COMMENT_STRINGS_MAP


def getCommentStyleForDoc(doc):
    ''' Get single line comment string for a document '''
    assert(doc.highlightingMode() in _COMMENT_STRINGS_MAP)
    return _COMMENT_STRINGS_MAP[doc.highlightingMode()]


def getBoundTextRangeSL(leftBoundary, rightBoundary, pos, doc):
    ''' Get the range between any symbol specified in leftBoundary set and rightBoundary

        Search starts from given cursor position...
        NOTE `SL' suffix means Single Line -- i.e. when search, do not cross one line boundaries!
    '''
    if not doc.lineLength(pos.line()):
        return KTextEditor.Range(pos, pos)

    lineStr = doc.line(pos.line())                          # Get the current line as string to analyse
    found = False
    for cc in range(pos.column(), -1, -1):                  # Moving towards line start
        found = lineStr[cc] in leftBoundary                 # Check the current char for left boundary terminators
        if found:
            break                                           # Break the loop if found smth

    startPos = KTextEditor.Cursor(pos.line(), cc + int(found))

    found = False
    for cc in range(pos.column(), len(lineStr)):            # Moving towards line end
        found = lineStr[cc] in rightBoundary                # Check the current char for right boundary terminators
        if found:
            break                                           # Break the loop if found smth

    endPos = KTextEditor.Cursor(pos.line(), cc + int(not found))

    return KTextEditor.Range(startPos, endPos)


def _getTextBlockAroundCursor(doc, first, last, direction, stopContidions):
    stop = False
    while (not stop and first >= 0 and first < last):
        line = doc.line(first)
        for predicate in stopContidions:
            if predicate(line):
                stop = True
                break
        if not stop:
            first += direction

    return first


TOWARDS_START = -1
TOWARDS_END = 1

def getTextBlockAroundCursor(doc, pos, upPred, downPred):
    start = _getTextBlockAroundCursor(doc, pos.line(), doc.lines(), TOWARDS_START, upPred)
    end = _getTextBlockAroundCursor(doc, pos.line(), doc.lines(), TOWARDS_END, downPred)
    if start != end:
        start += 1

    print('** getTextBlockAroundCursor[pos=%d,%d]: (%d,%d), (%d,%d)' % (pos.line(), pos.column(), start, 0, end, 0))
    return KTextEditor.Range(start, 0, end, 0)


def getLineIndentation(line, document):
    lineStr = document.line(line)
    return len(lineStr) - len(lineStr.lstrip())


def getCurrentLineIndentation(view):
    return getLineIndentation(view.cursorPosition().line(), view.document())


def extendSelectionToWholeLine(view):
    selectedRange = view.selectionRange()
    if not selectedRange.isEmpty():
        # ... try to extend selection to whole lines (of both ends), before do smth
        selectedRange.start().setColumn(0)
        if selectedRange.end().column() != 0:
            selectedRange.end().setColumn(0)
            selectedRange.end().setLine(selectedRange.end().line() + 1)
        view.setSelection(selectedRange)
