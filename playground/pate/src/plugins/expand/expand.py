
import kate
import kate.gui

import os
import sys
import re
import imp
import time
import traceback

from PyKDE4.kdecore import KConfig


class ParseError(Exception):
    pass

wordBoundary = set(u' \t"\';[]{}()#:/\\,+=!?%^|&*~`')

def wordAtCursor(document, view=None):
    view = view or document.activeView()
    cursor = view.cursorPosition()
    line = unicode(document.line(cursor.line()))
    start, end = wordAtCursorPosition(line, cursor)
    return line[start:end]

def wordAtCursorPosition(line, cursor):
    ''' Get the word under the active view's cursor in the given 
    document '''
    # better to use word boundaries than to hardcode valid letters because
    # expansions should be able to be in any unicode character.
    start = end = cursor.column()
    if start == len(line) or line[start] in wordBoundary:
        start -= 1
    while start >= 0 and line[start] not in wordBoundary:
        start -= 1
    start += 1
    while end < len(line) and line[end] not in wordBoundary:
        end += 1
    return start, end

def wordAndArgumentAtCursor(document, view=None):
    view = view or document.activeView()
    word_range, argument_range = wordAndArgumentAtCursorRanges(document, view.cursorPosition())
    if word_range.isEmpty():
        word = None
    else:
        word = unicode(document.text(word_range))
    if not argument_range or argument_range.isEmpty():
        argument = None
    else:
        argument = unicode(document.text(argument_range))
    return word, argument

def wordAndArgumentAtCursorRanges(document, cursor):
    line = unicode(document.line(cursor.line()))
    column_position = cursor.column()
    # special case: cursor past end of argument
    argument_range = None
    if cursor.column() > 0 and line[cursor.column() - 1] == ')':
        argument_end = kate.KTextEditor.Cursor(cursor.line(), cursor.column() - 1)
        argument_start = matchingParenthesisPosition(document, argument_end, opening=')')
        argument_end.setColumn(argument_end.column() + 1)
        argument_range = kate.KTextEditor.Range(argument_start, argument_end)
        cursor = argument_start
    line = unicode(document.line(cursor.line()))
    start, end = wordAtCursorPosition(line, cursor)
    word_range = kate.KTextEditor.Range(cursor.line(), start, cursor.line(), end)
    word = line[start:end]
    if argument_range is None and word:
        if end < len(line) and line[end] == '(':
            # ruddy lack of attribute type access from the KTextEditor
            # interfaces.
            argument_start = kate.KTextEditor.Cursor(cursor.line(), end)
            argument_end = matchingParenthesisPosition(document, argument_start, opening='(')
            argument_range = kate.KTextEditor.Range(argument_start, argument_end)
    return word_range, argument_range


def matchingParenthesisPosition(document, position, opening='('):
    closing = ')' if opening == '(' else '('
    delta = 1 if opening == '(' else -1
    # take a copy, Cursors are mutable
    position = position.__class__(position)
    
    level = 0
    state = None
    while 1:
        character = unichr(document.character(position).unicode())
        # print 'character:', repr(character)
        if state in ('"', "'"):
            if character == state:
                state = None
        else:
            if character == opening:
                level += 1
            elif character == closing:
                level -= 1
                if level == 0:
                    if closing == ')':
                        position.setColumn(position.column() + delta)
                    break
            elif character in ('"', "'"):
                state = character
        
        position.setColumn(position.column() + delta)
        # must we move down a line?
        if document.character(position).isNull():
            position.setPosition(position.line() + delta, 0)
            if delta == -1:
                # move to the far right
                position.setColumn(document.lineLength(position.line()) - 1)
            # failure again => EOF
            if document.character(position).isNull():
                raise ParseError('end of file reached')
            else:
                if state in ('"', "'"):
                    raise ParseError('end of line while searching for %s' % state)
    return position

# map of 'all' => {'name': func1, ....}
# map of 'mime/type' => {'name': func1, 'name2': func2}
expansionCache = {}


def loadFileExpansions(path):
    name = os.path.basename(path).split('.')[0]
    module = imp.load_source(name, path)
    expansions = {}
    # expansions are everything that don't begin with '__' and are callable
    for name in dir(module):
        o = getattr(module, name)
        # ignore builtins. Note that it is callable.__name__ that is used
        # to set the expansion key so you are free to reset it to something
        # starting with two underscores (or more importantly, a Python
        # keyword)
        if not name.startswith('__') and callable(o):
            expansions[o.__name__] = o
    return expansions

def loadExpansions(mime):
    if mime not in expansionCache:
        expansions = {}
        # explicit is better than implicit
        mimeFileName = mime.replace('/', '_') + '.expand'
        for directory in kate.applicationDirectories('expand'):
            if os.path.exists(os.path.join(directory, mimeFileName)):
                expansions.update(loadFileExpansions(os.path.join(directory, mimeFileName)))
        # load global expansions if necessary``
        expansionCache[mime] = loadExpansions('all') if mime != 'all' else {}
        expansionCache[mime].update(expansions)
    return expansionCache[mime]


def indentationCharacters(document):
    ''' The characters used to indent in a document as set by variables in the
    document or in the configuration. Will be something like '\t' or '    '
    '''
    v = document.variableInterface()
    # cache
    if not hasattr(indentationCharacters, 'configurationUseTabs'):
        config = KConfig('katerc')
        group = config.group('Kate Document Defaults')
        flags = str(group.readEntry('Basic Config Flags'))
        # gross, but there's no public API for this. Growl.
        indentationCharacters.configurationUseTabs = True
        if flags and int(flags) & 0x2000000:
            print 'insert spaces instead of tabulators'
            indentationCharacters.configurationUseTabs = False
        
        indentWidth = str(group.readEntry('Indentation Width'))
        indentationCharacters.configurationIndentWidth = int(indentWidth) if indentWidth else 4
    # indent with tabs or spaces
    useTabs = True
    spaceIndent = unicode(v.variable('space-indent'))
    if spaceIndent == 'on':
        useTabs = False
    elif spaceIndent == 'off':
        useTabs = True
    else:
        useTabs = indentationCharacters.configurationUseTabs
    
    if useTabs:
        return '\t'
    else:
        indentWidth = unicode(v.variable('indent-width'))
        if indentWidth and indentWidth.isdigit():
            return ' ' * int(indentWidth)
        else:
            # default
            return ' ' * indentationCharacters.configurationIndentWidth


@kate.action('Expand', shortcut='Ctrl+E', menu='Edit')
def expandAtCursor():
    document = kate.activeDocument()
    view = document.activeView()
    try:
        word_range, argument_range = wordAndArgumentAtCursorRanges(document, view.cursorPosition())
    except ParseError, e:
        kate.popup('Parse error:', e)
        return
    word = unicode(document.text(word_range))
    mime = str(document.mimeType())
    expansions = loadExpansions(mime)
    try:
        func = expansions[word]
    except KeyError:
        kate.gui.popup('Expansion "%s" not found :(' % word, timeout=3, icon='dialog-warning', minTextWidth=200)
        return
    argument = ()
    if argument_range is not None:
        # strip parentheses
        argument = (unicode(document.text(argument_range))[1:-1],)
        # map foo() => foo
        if argument == ('',):
            argument = ()
    # document.removeText(word_range)
    try:
        replacement = func(*argument)
    except Exception, e:
        # remove the top of the exception, it's our code
        try:
            type, value, tb = sys.exc_info()
            sys.last_type = type
            sys.last_value = value
            sys.last_traceback = tb
            tblist = traceback.extract_tb(tb)
            del tblist[:1]
            l = traceback.format_list(tblist)
            if l:
                l.insert(0, "Traceback (most recent call last):\n")
            l[len(l):] = traceback.format_exception_only(type, value)
        finally:
            tblist = tb = None
        # convert file names in the traceback to links. Nice.
        def replaceAbsolutePathWithLinkCallback(match):
            text = match.group()
            filePath = match.group(1)
            fileName = os.path.basename(filePath)
            text = text.replace(filePath, '<a href="%s">%s</a>' % (filePath, fileName))
            return text
        s = ''.join(l).strip()
        s = re.sub('File "(/[^\n]+)", line', replaceAbsolutePathWithLinkCallback, s)
        kate.gui.popup('<p style="white-space:pre">%s</p>' % s, icon='dialog-error', timeout=5, maxTextWidth=None, minTextWidth=300)
        return
    
    try:
        replacement = unicode(replacement)
    except UnicodeEncodeError:
        replacement = repr(replacement)
    #KateDocumentConfig::cfReplaceTabsDyn
    indentCharacters = indentationCharacters(document)
    # convert newlines followed by tab characters to whatever spacing
    # the user... uses.
    for i in xrange(100):
        if '\n' + (indentCharacters * i) + '\t' in replacement:
            replacement = replacement.replace('\n' + (indentCharacters * i) + '\t', '\n' + (indentCharacters * (i + 1)))
    insertPosition = word_range.start()
    line = unicode(document.line(insertPosition.line()))
    # autoindent: add the line's leading whitespace for each newline
    # in the expansion
    whitespace = ''
    for character in line:
        if character in ' \t':
            whitespace += character
        else:
            break
    replacement = replacement.replace('\n', '\n' + whitespace)
    # cursor position set?
    cursorAdvancement = None
    if '\1' in replacement:
        cursorAdvancement = replacement.index('\1')
        # strip around that byte
        replacement = replacement[:cursorAdvancement] + replacement[cursorAdvancement + 1:]
    # make the removal and insertion an atomic operation
    document.startEditing()
    if argument_range is not None:
        document.removeText(argument_range)
    document.removeText(word_range)
    document.insertText(insertPosition, replacement)
    # end before moving the cursor to avoid a crash
    document.endEditing()
    
    if cursorAdvancement is not None:
        # print 'advancing', cursorAdvancement
        smart = document.smartInterface().newSmartCursor(insertPosition)
        smart.advance(cursorAdvancement)
        view.setCursorPosition(smart)


# kate: space-indent on;
