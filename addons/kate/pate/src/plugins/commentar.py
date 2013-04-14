# -*- coding: utf-8 -*-
#
# Kate/Pâté plugins to work with C++ comments
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

'''Plugins to work with C++ comments'''

import kate
import kate.gui
import os
import re
import textwrap
from PyQt4 import uic
from PyQt4.QtGui import *
from PyKDE4.ktexteditor import KTextEditor

from libkatepate.decorators import *
from libkatepate import common, ui, selection
# text processing predicates
from libkatepate import pred
from libkatepate.pred import neg, all_of, any_of

BLOCK_ANY_START_SEARCH_RE = re.compile('^\s*#\s*if.*$')
BLOCK_START_SEARCH_RE = re.compile('^\s*#\s*if\s*([01])$')
BLOCK_ELSE_SEARCH_RE = re.compile('^\s*#\s*else.*$')
BLOCK_END_SEARCH_RE = re.compile('^\s*#\s*endif.*$')
BLOCK_ELSE_ENDIF_MATCH_RE = re.compile('^\s*#\s*(endif|else).*$')
BLOCK_START_GET_COND_RE = re.compile('^\s*#\s*(if((n)?def)?)\s+(.*)\s*$')


def buildIfEndifMap(document):
    '''
        Build a list of tuples (start, end, elseif, is_comment) for all #if/#elseif/#endif blocks
        in a document. If block contains #endif 3rd element will point its line, -1 otherwise.
        For blocks #if1/#if0 4th element is True, otherwise False.
    '''
    # Make list of ranges of #if*/#endif blocks
    openBlockStack = list()
    blockRanges = list()
    for i in range(0, document.lines()):
        currentLine = str(document.line(i))
        # Is #if block start?
        isComment = False
        matchObj = BLOCK_ANY_START_SEARCH_RE.search(currentLine)
        if bool(matchObj):
            matchObj = BLOCK_START_SEARCH_RE.search(currentLine)
            isComment = bool(matchObj)
            openBlockStack.append((True, i, isComment))
            continue
        # Is #else block?
        matchObj = BLOCK_ELSE_SEARCH_RE.search(currentLine)
        if bool(matchObj):
            openBlockStack.append((False, i, False))
            continue
        # Close block?
        matchObj = BLOCK_END_SEARCH_RE.search(currentLine)
        # Match? Is there any open block remain?
        if bool(matchObj) and bool(openBlockStack):
            isStart, blockLine, isComment = openBlockStack.pop()
            if isStart:
                blockRanges.append((blockLine, i, -1, isComment))
            else:
                isStart, start, isComment = openBlockStack.pop()
                blockRanges.append((start, i, blockLine, isComment))

    return blockRanges


def locateBlock(currentLine, blockRanges, ignoreComments = False):
    '''Find an index of a current block

        Return index into blockRanges of the block pointer by current cursor position
        or -1 if nothing found.

        TODO Rename to better name?
        TODO Is there any better way to implement a code similar to std::find_if in C++?
    '''
    # Locate a block where cursor currently positioned
    idx = -1
    c = 0
    for i in blockRanges:
        if i[0] <= currentLine and currentLine <= i[1] and (ignoreComments or i[3]):
            idx = c
            break
        c += 1
    return idx;


def processLine(line, commentCh):
    result = []
    column = kate.configuration["commentStartPos"]
    # Split line before and after a comment
    (before, comment, after) = line.partition(commentCh)
    before_s = before.rstrip()
    # Is there a comment on a line?
    if bool(comment):
        # Is there is any text before inline comment position?
        if bool(before_s):
            # Yes! Is text before not longer than desired comment position
            if len(before_s) < (kate.configuration["commentStartPos"] + 1):
                # Yep, just reformat the line...
                result.append(before_s + (' ' * (kate.configuration["commentStartPos"] - len(before_s))) + commentCh + after.rstrip())
            else:
                # Move comment to the line above
                column = len(before) - len(before.lstrip())
                # NOTE Try to fix Doxygen comment on the fly: '///<' or '//!<' --> '///'
                if after[1] == '<' and (after[0] == '!' or after[0] == '/'):
                    after = '/' + after[2:]
                result.append(' ' * column + commentCh + after.rstrip())
                result.append(before_s)
        else:
            # No! The line contains only whitespaces...
            # Is comment after or 'close before' to inline comment position?
            if len(before) > (kate.configuration["commentStartPos"] / 6):
                # Align comment to desired position...
                result.append(' ' * kate.configuration["commentStartPos"] + commentCh + after.rstrip())
            else:
                # TODO Align comment to closest to div 4 position...
                result.append(line.rstrip())
    else:
        # There is no comments... What about any text?
        if bool(before_s):
            # Is it longer that inline comment position?
            if len(before_s) > (kate.configuration["commentStartPos"]):
                column = len(before) - len(before.lstrip())
                result.append(' ' * column + commentCh + ' ')
                result.append(before_s)
            else:
                result.append(before_s + ' ' * (kate.configuration["commentStartPos"] - len(before_s)) + commentCh + ' ')
            # Check for preprocessor directives #else/#endif and try to append
            # corresponding #if condition as a comment for current line
            if bool(BLOCK_ELSE_ENDIF_MATCH_RE.search(before_s)):
                document = kate.activeDocument()
                view = kate.activeView()
                # Make list of ranges of #if*/#endif blocks
                blocksList = buildIfEndifMap(document)
                # Locate an index of a block where cursor currently positioned (check commented block too)
                idx = locateBlock(view.cursorPosition().line(), blocksList, True)
                # Get #if condition (if block located)
                if idx != -1:
                    # TODO Need to strip possible comment!
                    matchObj = BLOCK_START_GET_COND_RE.search(document.line(blocksList[idx][0]))
                    if bool(matchObj):
                        result[-1] += matchObj.group(4)
        else:
            # No text! Just add a comment...
            result.append(' ' * kate.configuration["commentStartPos"] + commentCh + ' ')
    return (result, column + len(commentCh) + 1)


@kate.action('Inline Comment', shortcut='Alt+D', menu='Edit')
@check_constraints
@comment_char_must_be_known()
@selection_mode(selection.NORMAL)
def commentar():
    ''' Append or align an inlined comment at position 60 for the current line or the selection.

        Move cursor to the start of a comment, if nothing has changed.
        If there wasn't any comment aside of #else/#endif put corresponding #if condition as default
        comment text
    '''
    document = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()
    commentCh = common.getCommentStyleForDoc(document)

    if view.selection():
        # If selected smth on a single line...
        common.extendSelectionToWholeLine(view)

        selectedText = view.selectionText().split('\n')
        if not bool(selectedText[-1]):
            selectedText = selectedText[0:-1]
        insertionText = []
        firstColumn = -1
        for textLine in selectedText:
            (currentLine, column) = processLine(textLine, commentCh)
            if firstColumn == -1:
                firstColumn = column
            insertionText += currentLine

        # Modify current document
        if bool(insertionText):
            document.startEditing()
            document.removeText(view.selectionRange())
            pos = view.cursorPosition()
            document.insertText(pos, '\n'.join(insertionText) + '\n')
            pos.setColumn(firstColumn)
            view.setCursorPosition(pos)
            view.removeSelection()
            document.endEditing()

    else:
        (text, column) = processLine(document.line(pos.line()), commentCh)

        # Apply result (if smth really has changed)
        originalText = document.line(pos.line())
        if bool(text) and (len(text) != 1 or originalText != text[0]):
            document.startEditing()                         # Start edit transaction:
            document.removeLine(pos.line())                 # Remove current line
            # insert resulting text line by line...
            pos.setColumn(0)
            document.insertText(pos, '\n'.join(text) + '\n')
            document.endEditing()                           # End transaction

        # Move cursor to desired position
        pos.setColumn(column)
        view.setCursorPosition(pos)


@kate.action('Move Comment Above', shortcut='Meta+Left')
@check_constraints
@comment_char_must_be_known()
def moveAbove():
    '''Move an inlined comment before the current line w/ same indentation level'''
    document = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()
    commentCh = common.getCommentStyleForDoc(document)

    insertionText = list()
    line = document.line(pos.line())
    # Split a line before and after a comment
    (before, comment, after) = str(line).partition(commentCh)

    before_ls = before.lstrip()
    column = len(before) - len(before_ls)
    doxCommentOffset = 0
    # Is there is a comment in a line?
    if bool(comment):
        # Yeah! It is... Now what about any text??
        if bool(before.strip()):
            if after[0:2] == '/<':
                after = '/' + after[2:]
                doxCommentOffset = 1
            insertionText.append(' ' * column + comment + after)
        else:
            # There is comment alone... Just leave it...
            return
    else:
        # Oops! There is no inline comment... Ok just add new one above.
        insertionText.append(' ' * column + commentCh)

    column += len(commentCh) + doxCommentOffset
    insertionText.append(before.rstrip());

    # Update the document
    if bool(insertionText):
        document.startEditing()                             # Start edit transaction:
        document.removeLine(pos.line())                     # Remove current line

        # insert resulting text line by line...
        pos.setColumn(0)
        document.insertText(pos, '\n'.join(insertionText) + '\n')

        # Move cursor to desired position
        pos.setColumn(column)
        view.setCursorPosition(pos)
        document.endEditing()                               # End transaction


@kate.action('Move Comment Inline', shortcut='Meta+Right')
@check_constraints
@comment_char_must_be_known()
def moveInline():
    ''' Move a comment at the current line as inline comment of the next line
        (if latter has no comment yet)
    '''
    document = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()
    commentCh = common.getCommentStyleForDoc(document)

    insertionText = []
    currentLine = document.line(pos.line())
    auxLine2Remove = 0
    # Split a line before and after a comment
    (before, comment, after) = currentLine.partition(commentCh)

    # Is there some text on a line?
    if bool(before.strip()):
        return                                              # Aha... move cursor co comment u stupid bastard!
    else:
        # No! What about comment?
        if bool(comment):
            # Aha... the comment is here. Ok. Lets get a line below the current...
            lineBelow = document.line(pos.line() + 1)
            (b_before, b_comment, b_after) = lineBelow.partition(commentCh)
            auxLine2Remove = 1
            # Check for text and comment in it...
            if bool(b_before.strip()):
                # Text present... Comment?
                if bool(b_comment):
                    # Comment too... just leave it...
                    return
                else:
                    # Just text.... no comment. Ok lets work!
                    # (if there is some space remains for inline comment)
                    b_before_s = b_before.rstrip()
                    if len(b_before_s) > kate.configuration["commentStartPos"]:
                        # Oops! No space remains! Get outa here
                        return
                    else:
                        doxCommentOffset = 0
                        if after[0:2] == '/ ':
                            after = '/< ' + after[2:]
                            doxCommentOffset = 2
                        insertionText.append(
                            b_before_s + ' ' * (kate.configuration["commentStartPos"] - len(b_before_s)) + commentCh + after.rstrip()
                            )
                        column = kate.configuration["commentStartPos"] + 3 + doxCommentOffset
            else:
                # No text on the line below! Dunno what damn user wants...
                return
        else:
            # Nothing! Just blank line... Dunno what to do...
            return
        pass

    # Update the document
    if bool(insertionText):
        document.startEditing()                             # Start edit transaction:
        if auxLine2Remove != 0:
            document.removeLine(pos.line() + auxLine2Remove)
        document.removeLine(pos.line())     # Remove current line

        # insert resulting text line by line...
        pos.setColumn(0)
        document.insertText(pos, '\n'.join(insertionText) + '\n')

        # Move cursor to desired position
        pos.setColumn(column)
        view.setCursorPosition(pos)
        document.endEditing()                               # End transaction


@kate.action('Comment Block w/ `#if0`', shortcut='Meta+D', menu='Edit')
@check_constraints
@restrict_doc_type('C++', 'C++11', 'C++11/Qt4', 'C')
def commentBlock():
    '''Wrap selected text (or current line) into a #if0/#endif block'''
    view = kate.activeView()

    # This operation have no sense for partly selected lines
    common.extendSelectionToWholeLine(view)

    start = -1
    end = -1
    if view.selection():
        sr = view.selectionRange()
        start = sr.start().line()
        end = sr.end().line() + 1
    else:
        start = view.cursorPosition().line()
        end = start + 2

    # Do it!
    document = kate.activeDocument()
    if start != -1 and end != -1:
        document.startEditing()                             # Start edit transaction
        document.insertLine(start, "#if 0")
        document.insertLine(end, "#endif")
        view.removeSelection()
        document.endEditing()                               # End transaction


@kate.action('Toggle `#if0/#if1` Block', shortcut='Meta+Shift+D', menu='Edit')
@check_constraints
@restrict_doc_type('C++', 'C++11', 'C++11/Qt4', 'C')
def toggleBlock():
    ''' Switch a current code block to ON(#if1) or OFF(#if0) state.

        Current means that cursor placed inside of it.
    '''
    document = kate.activeDocument()
    view = kate.activeView()

    # Make list of ranges of #if*/#endif blocks
    blocksList = buildIfEndifMap(document)

    # Locate a block where cursor currently positioned
    idx = locateBlock(view.cursorPosition().line(), blocksList, False)

    if idx != -1:
        # Get current value
        v = BLOCK_START_SEARCH_RE.search(str(document.line(blocksList[idx][0]))).group(1)
        # Toggle it!
        if v in ('0', 'false'):
            newValue = '1'
        elif v in ('1', 'true'):
            newValue = '0'
        else:
            return

        # Replace string
        document.startEditing()                                 # Start edit transaction
        document.removeLine(blocksList[idx][0])
        # TODO Do not lose formatting!
        document.insertLine(blocksList[idx][0], "#if " + newValue)
        document.endEditing()                                   # End transaction
    else:
        ui.popup("Oops", "It seems cursor positioned out of any #if0/#if1 block", "face-sad")


@kate.action('Remove `#if0` Block', shortcut='Meta+R', menu='Edit')
@check_constraints
@restrict_doc_type('C++', 'C++11', 'C++11/Qt4', 'C')
def removeBlock():
    ''' Remove a block of code commented with #if0 or #if1-#else'''
    document = kate.activeDocument()
    view = kate.activeView()

    # Make list of ranges of #if*/#endif blocks
    blocksList = buildIfEndifMap(document)

    # Locate a block where cursor currently positioned
    idx = locateBlock(view.cursorPosition().line(), blocksList, False)

    if idx != -1:
        # Get current value
        v = BLOCK_START_SEARCH_RE.search(str(document.line(blocksList[idx][0]))).group(1)
        # Do nothing if it's not a #if0/#if1
        if v not in ('0', 'false', '1', 'true'):
            return

        document.startEditing()                             # Start edit transaction
        # What to remove?
        if v in ('0', 'false'):                             # Remove `then` part
            if blocksList[idx][2] != -1:                    # Is there `#else` part?
                # Yeah! Remove `#endif` line and then from `#if` to `#else` (including)
                document.removeLine(blocksList[idx][1])
                r = KTextEditor.Range(blocksList[idx][0], 0, blocksList[idx][2] + 1, 0)
            else:
                # No! So just remove whole block
                r = KTextEditor.Range(blocksList[idx][0], 0, blocksList[idx][1] + 1, 0)
            document.removeText(r)
        else:
            if blocksList[idx][2] != -1:                    # Is there `#else` part?
                # Yeah! Remove from `#else` to `#endif` block and then `#if` line
                r = KTextEditor.Range(blocksList[idx][2], 0, blocksList[idx][1] + 1, 0)
                document.removeText(r)
                document.removeLine(blocksList[idx][0])
            else:
                # No! Ok just remove `#endif` line and then `#if`
                document.removeLine(blocksList[idx][1])
                document.removeLine(blocksList[idx][0])
        document.endEditing()                                   # End transaction
    else:
        ui.popup("Oops", "It seems cursor positioned out of any #if0/#if1 block", "face-sad")


@kate.action('Select Current `#if0/#if1` Block', shortcut='Meta+S', menu='Edit')
@check_constraints
@restrict_doc_type('C++', 'C++11', 'C++11/Qt4', 'C')
def selectBlock():
    '''Set selection of a current (where cursor positioned) #if0/#endif block'''
    document = kate.activeDocument()
    view = kate.activeView()

    # Make list of ranges of #if*/#endif blocks
    blocksList = buildIfEndifMap(document)

    # Locate a block where cursor currently positioned
    idx = locateBlock(view.cursorPosition().line(), blocksList)

    if idx != -1:
        r = KTextEditor.Range(blocksList[idx][0], 0, blocksList[idx][1] + 1, 0)
        view.setSelection(r)
    else:
        ui.popup("Oops", "It seems cursor positioned out of any #if0/#if1 block", "face-sad")


def turnToBlockComment():
    document = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()

    if view.selection():
        sr = view.selectionRange()
        start = sr.start().line()
        end = sr.end().line()
    else:
        r = common.getTextBlockAroundCursor(
            document
          , pos
          , [neg(any_of(pred.startsWith('///'), pred.startsWith('//!')))]
          , [neg(any_of(pred.startsWith('///'), pred.startsWith('//!')))]
          )
        start = r.start().line()
        end = r.end().line()

    # Replace comments in every line
    insertionText = list()
    align = None
    for i in range(start, end):
        line = str(document.line(i))
        sline = line.lstrip()
        if align == None:
            align = ' ' * (len(line) - len(sline))
        insertionText.append(align + sline.replace('///', ' *', 1).replace('//!', ' *', 1))

    originRange = KTextEditor.Range(start, 0, end, 0)
    pos.setPosition(start + 1, len(align) + 3)
    insertPos = KTextEditor.Cursor(start, 0)

    # Update the document
    if bool(insertionText):
        document.startEditing()                             # Start edit transaction:
        document.removeText(originRange)                    # Remove current line

        # insert resulting text ...
        document.insertText(insertPos, align + '/**\n' + '\n'.join(insertionText) + '\n' + align + ' */\n');

        # Move cursor to desired position
        view.setCursorPosition(pos)
        document.endEditing()                               # End transaction


def turnFromBlockComment():
    document = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()

    if view.selection():
        sr = view.selectionRange()
        start = sr.start().line()
        end = sr.end().line()
    else:
        # Try to detect block comment (/* ... */)
        r = common.getTextBlockAroundCursor(
            document
          , pos
          , [pred.blockCommentStart, neg(pred.startsWith('*'))]
          , [pred.blockCommentEnd, neg(pred.startsWith('*'))]
          )

        start = r.start().line() - 1
        end = r.end().line() + 1

    # Replace comments
    insertionText = list()
    align = None
    for i in range(start, end):
        line = str(document.line(i))
        sline = line.lstrip()
        if align == None:
            align = ' ' * (len(line) - len(sline))
        if sline.startswith('/**') or sline.startswith('*/'):
            continue
        if sline.startswith('*'):
            insertionText.append(align + sline.replace('*', '///', 1))

    originRange = KTextEditor.Range(start, 0, end, 0)
    pos.setPosition(start, len(align) + 3)
    insertPos = KTextEditor.Cursor(start, 0)

    # Update the document
    if bool(insertionText):
        document.startEditing()                             # Start edit transaction:
        document.removeText(originRange)                    # Remove current line

        # insert resulting text line by line...
        document.insertText(insertPos, '\n'.join(insertionText) + '\n')

        # Move cursor to desired position
        view.setCursorPosition(pos)
        document.endEditing()                               # End transaction


@kate.action('Transform Doxygen Comments', shortcut='Meta+X', menu='Edit')
@check_constraints
@restrict_doc_type('C++', 'C++11', 'C++11/Qt4', 'JavaScript')
@has_selection(False)
def toggleDoxyComment():
    ''' Turn block of '///' doxygen comments into
        /**
         *
         */
        and vise versa
    '''
    document = kate.activeDocument()
    view = kate.activeView()
    pos = view.cursorPosition()

    # Determine type of current comment
    line = str(document.line(pos.line())).strip()
    if line.startswith('///'):
        turnToBlockComment()
    elif line.startswith('*') or line.startswith('/*') or line.startswith('*/'):
        turnFromBlockComment()
    else:
        return


def getParagraphRange(doc, pos):
    # Try to detect block comment (/* ... */)
    r = common.getTextBlockAroundCursor(
        doc
      , pos
      , [pred.blockCommentStart, pred.equalTo('*'), neg(pred.startsWith('*'))]
      , [pred.blockCommentEnd, pred.equalTo('*'), neg(pred.startsWith('*'))]
      )
    isBlock = True
    if r.isEmpty():
        # Ok, maybe it's a single lines comment block?
        r = common.getTextBlockAroundCursor(
            doc
          , pos
          , [neg(pred.onlySingleLineComment)]
          , [neg(pred.onlySingleLineComment)]
          )
        isBlock = False
    return (r, isBlock)


def changeParagraphWidth(step):
    view = kate.activeView()
    doc = kate.activeDocument()
    pos = view.cursorPosition()

    originRange, isBlock = getParagraphRange(doc, pos)
    if originRange.isEmpty():
        ui.popup("Sorry", "can't detect commented paragraph at cursor...", "face-sad")
        return                                              # Dunno what to do on empty range!

    indent = common.getCurrentLineIndentation(view)         # detect current align
    # Processing:
    # 0) split text into left stripped lines
    originalText = view.document().text(originRange)
    lines = [line.lstrip() for line in originalText.split('\n')]
    # 1) detect comment style
    comment = [c.strip() for c in lines[0].split(' ')][0]
    # 2) strip leading comments (and possible left spaces) from each line
    lines = [line[len(comment):].lstrip() for line in lines]
    # 3) get a desired width of the current paragraph
    if step == -1:
        # 3.1) For shrink it is really simple: we just want to fit last word
        # to the next line, and it is enough to specify max(line size) - 1
        newSize = len(max(lines, key=len)) - 1
    elif step == 1:
        # 3.2) To extend paragraph we just want to append a first word from the next
        # after longest line.
        currentMax = 0
        prevLineWasLongest = False
        delta = 0
        for line in lines:
            # 3.2.1) if current maximum was changed on prevoius iteration,
            # get length of a first word on a line
            if prevLineWasLongest:
                # NOTE +1 for one space
                delta = len([word.strip() for word in line.split(' ')][0]) + 1
            # 3.2.2) is current line longer than we've seen before?
            lineSize = len(line)
            prevLineWasLongest = bool(currentMax < lineSize)
            if prevLineWasLongest:
                currentMax = lineSize
        newSize = currentMax + delta
    else:
        assert(not "Incorrect step specified")

    # 4) wrap the text
    res = textwrap.wrap(' '.join(lines), newSize, break_long_words=False)
    # 5) form a text from the result list
    align = ' ' * indent + comment + ' '
    text = align + ('\n' + align).join(res) + '\n'

    # Return text only if smth really changed
    if originalText != text:                                # Update document only if smth really has changed
        doc.startEditing()                                  # Start edit transaction:
        doc.removeText(originRange)                         # Remove the origin range
        doc.insertText(originRange.start(), text)           # Insert modified text
        view.setCursorPosition(originRange.start())         # Move cursor to the start of the origin range
        doc.endEditing()                                    # End transaction


@kate.action('Shrink Comment Paragraph', shortcut='Meta+[', menu='Edit')
@check_constraints
@restrict_doc_type('C++', 'C++11', 'C++11/Qt4', 'JavaScript')
def shrinkParagraph():
    '''Shrink a text paragraph width, whithing a comment, around the current cursor position'''
    changeParagraphWidth(-1)


@kate.action('Extend Comment Paragraph', shortcut='Meta+]', menu='Edit')
@check_constraints
@restrict_doc_type('C++', 'C++11', 'C++11/Qt4', 'JavaScript')
def extendParagraph():
    '''Extend a text paragraph width, whithing a comment, around the current cursor position'''
    changeParagraphWidth(1)


# ----------------------------------------------------------
# Plugin configuration stuff
# ----------------------------------------------------------

class ConfigWidget(QWidget):
    """Configuration widget for this plugin."""
    #
    # Same-line-comment start position
    #
    commentStartPos = None

    def __init__(self, parent=None, name=None):
        super(ConfigWidget, self).__init__(parent)

        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), "commentar_config.ui"), self)

        self.reset();

    def apply(self):
        kate.configuration["commentStartPos"] = self.commentStartPos.value()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if "commentStartPos" in kate.configuration:
            self.commentStartPos.setValue(kate.configuration["commentStartPos"])

    def defaults(self):
        self.commentStartPos.setValue(60)


class ConfigPage(kate.Kate.PluginConfigPage, QWidget):
    """Kate configuration page for this plugin."""
    def __init__(self, parent = None, name = None):
        super(ConfigPage, self).__init__(parent, name)
        self.widget = ConfigWidget(parent)
        lo = parent.layout()
        lo.addWidget(self.widget)

    def apply(self):
        self.widget.apply()

    def reset(self):
        self.widget.reset()

    def defaults(self):
        self.widget.defaults()
        self.changed.emit()


@kate.configPage("Commentar Plugin", "Commentar Plugin Settings", icon="preferences-other")
def commentarConfigPage(parent=None, name=None):
    return ConfigPage(parent, name)

@kate.init
def init():
    # Set default value if not configured yet
    if "commentStartPos" not in kate.configuration:
        kate.configuration["commentStartPos"] = 60

# kate: indent-width 4;
