# -*- coding: utf-8 -*-
#
# Kate/Pâté plugins to work with C++ code formatting
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

'''Plugins to work with C++ code formatting'''

from PyKDE4.kdecore import i18nc
from PyKDE4.ktexteditor import KTextEditor
from PyKDE4.kdeui import KXMLGUIClient

import kate
import kate.ui
import kate.view

from libkatepate.decorators import *
from libkatepate import selection


def getLeftNeighbour(lineStr, column):
    if column:
        return lineStr[column - 1]
    return None

def getRightNeighbour(lineStr, column):
    if (column + 1) < len(lineStr):
        return lineStr[column + 1]
    return None

def looksLikeTemplateAngelBracket(lineStr, column):
    ''' Check if a symbol at given position looks like a template angel bracket
    '''
    assert(lineStr[column] in '<>')
    #kate.qDebug("?LLTAB: ch='" + lineStr[column] + "'")
    ln = getLeftNeighbour(lineStr, column)
    #kate.qDebug("?LLTAB: ln='" + str(ln) + "'")
    rn = getRightNeighbour(lineStr, column)
    #kate.qDebug("?LLTAB: rn='" + str(rn) + "'")
    # Detect possible template
    if lineStr[column] == '<':                              # --[current char is '<']-------
        if ln == '<' or rn == '<':                          # "<<" in any place on a line...
            return False                                    # ... can't be a template!
        if ln == ' ' and rn == '=':                         # " <="
            return False                                    # operator<=()
        if lineStr[0:column].strip().startswith('template'):# template declaration at the start of line
            return True                                     # ... possible smth like `template < typename ...'
        if ln == ' ' and rn == ' ':                         # " < "
            return False                                    # operator<()
        return True
    if lineStr[column] == '>':                              # --[current char is '>']-------
        if lineStr.strip().startswith('>'):                 # line starts w/ one or more '>'
            return True                                     # ... can be end of formatted `typedef <...\n> type;' for example
        if ln == ' ' and rn == ' ':                         # " > "
            return False                                    # operator>()
        if ln == ' ' and rn == '=':                         # ">="
            return False                                    # operator>=()
        if ln == '-':
            return False                                    # operator->()
        return True
    pass

#
# TODO Probably decorators may help to simplify this code ???
#
def getRangeTopology(breakChars):
    '''Get range opened w/ `openCh' and closed w/ `closeCh'

        @return tuple w/ current range, list of nested ranges
                and list of positions of break characters

        @note Assume cursor positioned whithin that range already.
    '''
    document = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()

    stack = list()
    nestedRanges = list()
    breakPositions = list()
    firstIteration = True
    found = False
    # Iterate from the current line towards a document start
    for cl in range(pos.line(), -1, -1):
        lineStr = str(document.line(cl))
        if not firstIteration:                              # skip first iteration
            pos.setColumn(len(lineStr))                     # set current column to the end of current line
        else:
            firstIteration = False                          # do nothing on first iteration
        # Iterate from the current column to a line start
        for cc in range(pos.column() - 1, -1, -1):
            #kate.qDebug("c: current position" + str(cl) + "," + str(cc) + ",ch='" + lineStr[cc] + "'")
            # Check open/close brackets
            if lineStr[cc] == ')':                          # found closing char: append its position to the stack
                stack.append((cl, cc, False))
                #kate.qDebug("o( Add position: " + str(stack[-1]))
                continue
            if lineStr[cc] == '(':                          # found open char...
                if len(stack):                              # if stack isn't empty (i.e. there are some closing chars met)
                    #kate.qDebug("o( Pop position: " + str(stack[-1]))
                    nrl, nrc, isT = stack.pop()             # remove last position from the stack
                    if not isT:
                        nestedRanges.append(                # and append a nested range
                            KTextEditor.Range(cl, cc, nrl, nrc)
                          )
                    else:
                        raise LookupError(
                            i18nc(
                                '@info'
                              , 'Misbalanced brackets: at <numid>%1</numid>,<numid>%2</numid> and <numid>%3</numid>,<numid>%4</numid>'
                              , cl + 1, cc + 1, nrl + 1, nrc + 1
                              )
                          )
                else:                                       # otherwise,
                    openPos = (cl, cc + 1, False)           # remember range start (exclude an open char)
                    #kate.qDebug("o( Found position: " + str(openPos))
                    found = True
                    break
                continue
            # Check for template angel brackets
            if lineStr[cc] == '>':
                if looksLikeTemplateAngelBracket(lineStr, cc):
                    stack.append((cl, cc, True))
                    #kate.qDebug("o< Add position: " + str(stack[-1]))
                #else:
                    #kate.qDebug("o< Doesn't looks like template: " + str(cl) + "," + str(cc))
                continue
            if lineStr[cc] == '<':
                if not looksLikeTemplateAngelBracket(lineStr, cc):
                    #kate.qDebug("o< Doesn't looks like template: " + str(cl) + "," + str(cc + 1))
                    pass
                elif len(stack):                            # if stack isn't empty (i.e. there are some closing chars met)
                    #kate.qDebug("o< Pop position: " + str(stack[-1]))
                    nrl, nrc, isT = stack.pop()             # remove last position from the stack
                    if isT:
                        nestedRanges.append(                # and append a nested range
                            KTextEditor.Range(cl, cc, nrl, nrc)
                        )
                    else:
                        raise LookupError(
                            i18nc(
                                '@info'
                              , 'Misbalanced brackets: at <numid>%1</numid>,<numid>%2</numid> and <numid>%3</numid>,<numid>%4</numid>'
                              , cl + 1, cc + 1, nrl + 1, nrc + 1
                              )
                          )
                else:
                    openPos = (cl, cc + 1, True)            # remember range start (exclude an open char)
                    #kate.qDebug("o< Found position: " + str(openPos))
                    found = True
                    break
                continue
            if lineStr[cc] in breakChars and len(stack) == 0:
                breakPositions.append(KTextEditor.Cursor(cl, cc))
        # Did we found smth on the current line?
        if found:
            break                                           # Yep! Break the outer loop

    if not found:
        return (KTextEditor.Range(), list(), list())        # Return empty ranges if nothing found

    assert(len(stack) == 0)                                 # stack expected to be empty!

    breakPositions.reverse()                                # reverse breakers list required cuz we found 'em in a reverse order :)

    # Iterate from the current position towards the end of a document
    pos = view.cursorPosition()                             # get current cursor position again
    firstIteration = True
    found = False
    for cl in range(pos.line(), document.lines()):
        lineStr = str(document.line(cl))
        if not firstIteration:                              # skip first iteration
            pos.setColumn(0)                                # set current column to the start of current line
        else:
            firstIteration = False                          # do nothing on first iteration
        for cc in range(pos.column(), len(lineStr)):
            #kate.qDebug("c: current position" + str(cl) + "," + str(cc) + ",ch='" + lineStr[cc] + "'")
            # Check open/close brackets
            if lineStr[cc] == '(':
                stack.append((cl, cc, False))
                #kate.qDebug("c) Add position: " + str(stack[-1]))
                continue
            if lineStr[cc] == ')':
                if len(stack):
                    #kate.qDebug("c) Pop position: " + str(stack[-1]))
                    nrl, nrc, isT = stack.pop()             # remove a last position from the stack
                    if not isT:
                        nestedRanges.append(                # and append a nested range
                            KTextEditor.Range(nrl, nrc, cl, cc)
                        )
                    else:
                        raise LookupError(
                            i18nc(
                                '@info'
                              , 'Misbalanced brackets: at <numid>%1</numid>,<numid>%2</numid> and <numid>%3</numid>,<numid>%4</numid>'
                              , nrl + 1, nrc + 1, cl + 1, cc + 1
                              )
                          )
                else:
                    closePos = (cl, cc, False)              # remember the range end
                    #kate.qDebug("c) Found position: " + str(closePos))
                    found = True
                    break
                continue
            # Check for template angel brackets
            if lineStr[cc] == '<':
                if looksLikeTemplateAngelBracket(lineStr, cc):
                    stack.append((cl, cc, True))
                    #kate.qDebug("c> Add position: " + str(stack[-1]))
                #else:
                    #kate.qDebug("c> Doesn't looks like template: " + str(cl) + "," + str(cc))
                continue
            if lineStr[cc] == '>':
                if not looksLikeTemplateAngelBracket(lineStr, cc):
                    #kate.qDebug("c> Doesn't looks like template: " + str(cl) + "," + str(cc))
                    pass
                elif len(stack):                            # if stack isn't empty (i.e. there are some closing chars met)
                    #kate.qDebug("c> Pop position: " + str(stack[-1]))
                    nrl, nrc, isT = stack.pop()             # remove last position from the stack
                    if isT:
                        nestedRanges.append(                # and append a nested range
                            KTextEditor.Range(cl, cc, nrl, nrc)
                        )
                    else:
                        raise LookupError(
                            i18nc(
                                '@info'
                              , 'Misbalanced brackets: at <numid>%1</numid>,<numid>%2</numid> and <numid>%3</numid>,<numid>%4</numid>'
                              , nrl + 1, nrc + 1, cl + 1, cc + 1
                              )
                          )
                else:
                    closePos = (cl, cc, True)               # remember the range end
                    kate.qDebug("c> Found position: " + str(closePos))
                    found = True
                    break
                continue
            if lineStr[cc] in breakChars and len(stack) == 0:
                breakPositions.append(KTextEditor.Cursor(cl, cc))
        # Did we found smth on the current line?
        if found:
            break                                           # Yep! Break the outer loop

    if not found:
        return (KTextEditor.Range(), list(), list())        # Return empty ranges if nothing found

    assert(len(stack) == 0)                                 # stack expected to be empty!

    if openPos[2] != closePos[2]:
        raise LookupError(
            i18nc(
                '@info'
              , 'Misbalanced brackets: at <numid>%1</numid>,<numid>%2</numid> and <numid>%3</numid>,<numid>%4</numid>'
              , openPos[0] + 1, openPos[1] + 1, closePos[0] + 1, closePos[1] + 1
              )
          )

    return (KTextEditor.Range(openPos[0], openPos[1], closePos[0], closePos[1]), nestedRanges, breakPositions)


def boostFormatText(textRange, indent, breakPositions):
    document = kate.activeDocument()
    originalText = document.text(textRange)
    #kate.qDebug("Original text:\n'" + originalText + "'")

    # Slice text whithin a given range into pieces to be realigned
    ranges = list()
    prevPos = textRange.start()
    breakCh = None
    indentStr = ' ' * (indent + 2);
    breakPositions.append(textRange.end())
    for b in breakPositions:
        #kate.qDebug("* prev pos: " + str(prevPos.line()) + ", " + str(prevPos.column()))
        #kate.qDebug("* current pos: " + str(b.line()) + ", " + str(b.column()))
        chunk = (document.text(KTextEditor.Range(prevPos, b))).strip()
        #kate.qDebug("* current chunk:\n'" + chunk + "'")
        t = ('\n    ').join(chunk.splitlines())
        #kate.qDebug("* current line:\n'" + t + "'")
        if breakCh:
            outText += indentStr + breakCh + ' ' + t + '\n'
        else:
            outText = '\n' + indentStr + '  ' + t + '\n'

        breakCh = document.character(b)
        prevPos = KTextEditor.Cursor(b.line(), b.column() + 1)

    outText += indentStr

    #kate.qDebug("Out text:\n'" + outText + "'")
    if outText != originalText:
        document.startEditing()
        document.replaceText(textRange, outText)
        document.endEditing()

@kate.action
@check_constraints
@selection_mode(selection.NORMAL)
def boostFormat():
    '''Format function's/template's parameters list (or `for`'s) in a boost-like style
       I.e. when 2nd and the rest parameters has leading comma/semicolon
       and closing ')' or '>' on a separate line.
       THIS IS REALLY BETTER TO HAVE SUCH STYLE WHEN U HAVE A LONG PARAMETERS LIST!
    '''
    document = kate.activeDocument()
    view = kate.activeView()

    try:
        r, nestedRanges, breakPositions = getRangeTopology(',')
    except LookupError as error:
        kate.ui.popup(
            i18nc('@title:window', 'Alert')
          , i18nc(
                '@info:tooltip'
              , 'Failed to parse C++ expression:<nl/><message>%1</message>', error
              )
          , 'dialog-information'
          )
        return

    if r.isEmpty():                                         # Is range empty?
        kate.ui.popup(
            i18nc('@title:window', 'Alert')
          , i18nc(
                '@info:tooltip'
              , 'Failed to parse C++ expression:<nl/><message>%1</message>'
              , i18nc('@info:tooltip', "Did not find anything to format")
              )
          , 'dialog-information'
          )
        return                                              # Nothing interesting wasn't found...

    # Rescan the range w/ ';' as breaker added if current range is a `for` statement
    if document.line(r.start().line())[0:r.start().column() - 1].rstrip().endswith('for'):
        try:
            r, nestedRanges, breakPositions = getRangeTopology(',;')
        except LookupError as error:
            kate.ui.popup(
                i18nc('@title:window', 'Alert')
              , i18nc(
                    '@info:tooltip'
                  , 'Failed to parse C++ expression:<nl/><message>%1</message>', error
                  )
              , 'dialog-information'
              )
            return

    # Going to format a text whithin a selected range
    lineStr = document.line(r.start().line())
    lineStrStripped = lineStr.lstrip()
    indent = len(lineStr) - len(lineStrStripped)
    if lineStrStripped.startswith(', '):
        indent += 2
    text = boostFormatText(r, indent, breakPositions)


def boostUnformatText(textRange, breakPositions):
    document = kate.activeDocument()
    originalText = document.text(textRange)
    #kate.qDebug("Original text:\n'" + originalText + "'")

    # Join text within a selected range
    prevPos = textRange.start()
    outText = ''.join([line.strip() for line in originalText.splitlines()])

    #kate.qDebug("Out text:\n'" + outText + "'")
    if outText != originalText:
        document.startEditing()
        document.replaceText(textRange, outText)
        document.endEditing()


@kate.action
@check_constraints
@selection_mode(selection.NORMAL)
def boostUnformat():
    '''Merge everything between '(' and ')' into a single line'''
    document = kate.activeDocument()
    view = kate.activeView()

    try:
        r, nestedRanges, breakPositions = getRangeTopology(',')
    except LookupError as error:
        kate.ui.popup(
            i18nc('@title:window', 'Alert')
          , i18nc(
                '@info:tooltip'
              , 'Failed to parse C++ expression:<nl/><message>%1</message>', error
              )
          , 'dialog-information'
          )
        return

    if r.isEmpty():                                         # Is range empty?
        kate.ui.popup(
            i18nc('@title:window', 'Alert')
          , i18nc(
                '@info:tooltip'
              , 'Failed to parse C++ expression:<nl/><message>%1</message>'
              , i18nc('@info:tooltip', "Did not find anything to format")
              )
          , 'dialog-information'
          )
        return                                              # Nothing interesting wasn't found...

    # Rescan the range w/ ';' as breaker added if current range is a `for` statement
    if document.line(r.start().line())[0:r.start().column() - 1].rstrip().endswith('for'):
        try:
            r, nestedRanges, breakPositions = getRangeTopology(',;')
        except LookupError as error:
            kate.ui.popup(
                i18nc('@title:window', 'Alert')
              , i18nc(
                    '@info:tooltip'
                  , 'Failed to parse C++ expression:<nl/><message>%1</message>', error
                  )
              , 'dialog-information'
              )
            return

    # Going to unformat a text whithin a selected range
    text = boostUnformatText(r, breakPositions)


@kate.view.selectionChanged
def toggleSelectionSensitiveActions(view):
    clnt = kate.getXmlGuiClient()
    if not view.selection():
        clnt.stateChanged('has_no_selection')
    else:
        clnt.stateChanged('has_no_selection', KXMLGUIClient.StateReverse)
