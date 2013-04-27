
import kate
from libkatepate import ui, common

from PyKDE4.kdecore import i18nc

import re


def openingTagBeforeCursor(document, position):
    currentLine = document.line(position.line())
    currentLine = currentLine[:position.column()].rstrip()
    tag = re.compile('<\s*([^/][^ ]*)(?:\s+[^>]+)?>')
    openTags = list(tag.finditer(currentLine))
    if openTags:
        lastMatch = max(openTags, key=lambda x: x.end())
        if lastMatch.end() == len(currentLine):
            return lastMatch.group(1)
    return None


@kate.action(i18nc('@action:inmenu', 'Close Tag'), shortcut='Ctrl+Shift+K', menu='Edit')
def closeTagAtCursor():
    document = kate.activeDocument()
    view = document.activeView()
    currentPosition = view.cursorPosition()
    insertionPosition = view.cursorPosition()
    tag = openingTagBeforeCursor(document, insertionPosition)
    onPreviousLine = False
    tagLine = None
    if tag is None:
        insertionPosition.setLine(insertionPosition.line() - 1)
        if insertionPosition.isValid():
            insertionPosition.setColumn(document.lineLength(insertionPosition.line()))
            if insertionPosition.isValid():
                tag = openingTagBeforeCursor(document, insertionPosition)
                tagLine = document.line(insertionPosition.line())
                onPreviousLine = True
                insertionPosition.setLine(currentPosition.line() + 1)
                insertionPosition.setColumn(0)
    if tag is None:
        ui.popup(
            i18nc('@title:window', 'Parse error')
          , i18nc('@info:tooltip', 'No opening tag found')
          , 'dialog-warning'
          )
        return

    currentLine = document.line(currentPosition.line())
    insertionText = '</%s>' % tag
    if onPreviousLine:
        leadingSpacing = re.search('^\s*', tagLine).group(0)
        insertionText = '%s%s\n' % (leadingSpacing, insertionText)
    document.startEditing()
    document.insertText(insertionPosition, insertionText)
    view.setCursorPosition(currentPosition)
    document.endEditing()

# kate: space-indent on; indent-width 4;
