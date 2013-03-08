"""User-defined text expansions.

Each text expansion is a simple function which must return a string.
This string will be inserted into a document by the expandAtCursor action.
For example if you have a function "foo" in then all.expand file
which is defined as:

def foo:
  return 'Hello from foo!'

after typing "foo", the action will replace "foo" with "Hello from foo!".
The expansion function may have parameters as well. For example, the
text_x-c++src.expand file contains the "cl" function which creates a class
(or class template). This will expand "cl(test)" into:

/**
 * \\brief Class \c test
 */
class test
{
public:
    /// Default constructor
    explicit test()
    {
    }
    /// Destructor
    virtual ~test()
    {
    }
};

but "cl(test, T1, T2, T3)" will expand to:

/**
 * \\brief Class \c test
 */
template <typename T1, typename T2, typename T3>
class test
{
public:
    /// Default constructor
    explicit test()
    {
    }
    /// Destructor
    virtual ~test()
    {
    }
};
"""
# -*- coding: utf-8 -*-

import kate

import os
import sys
import re
import imp
import time
import traceback

from PyKDE4.kdecore import KConfig

from libkatepate import ui, common


class ParseError(Exception):
    pass


def wordAndArgumentAtCursorRanges(document, cursor):
    line = document.line(cursor.line())

    argument_range = None
    # Handle some special cases:
    if cursor.column() > 0:
        if cursor.column() < len(line) and line[cursor.column()] == ')':
            # special case: cursor just right before a closing brace
            argument_end = kate.KTextEditor.Cursor(cursor.line(), cursor.column())
            argument_start = matchingParenthesisPosition(document, argument_end, opening=')')
            argument_end.setColumn(argument_end.column() + 1)
            argument_range = kate.KTextEditor.Range(argument_start, argument_end)
            cursor = argument_start                         #  NOTE Reassign

        if line[cursor.column() - 1] == ')':
            # one more special case: cursor past end of arguments
            argument_end = kate.KTextEditor.Cursor(cursor.line(), cursor.column() - 1)
            argument_start = matchingParenthesisPosition(document, argument_end, opening=')')
            argument_end.setColumn(argument_end.column() + 1)
            argument_range = kate.KTextEditor.Range(argument_start, argument_end)
            cursor = argument_start                         #  NOTE Reassign

    word_range = common.getBoundTextRangeSL(
        common.IDENTIFIER_BOUNDARIES
      , common.IDENTIFIER_BOUNDARIES
      , cursor
      , document
      )
    end = word_range.end().column()
    if argument_range is None and word_range.isValid():
        if end < len(line) and line[end] == '(':
            # ruddy lack of attribute type access from the KTextEditor
            # interfaces.
            argument_start = kate.KTextEditor.Cursor(cursor.line(), end)
            argument_end = matchingParenthesisPosition(document, argument_start, opening='(')
            argument_range = kate.KTextEditor.Range(argument_start, argument_end)
    return word_range, argument_range

# TODO Generalize this and move to `common' package
def matchingParenthesisPosition(document, position, opening='('):
    closing = ')' if opening == '(' else '('
    delta = 1 if opening == '(' else -1
    # take a copy, Cursors are mutable
    position = position.__class__(position)

    level = 0
    state = None
    while 1:
        character = document.character(position)
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
        if document.character(position) == None:
            position.setPosition(position.line() + delta, 0)
            if delta == -1:
                # move to the far right
                position.setColumn(document.lineLength(position.line()) - 1)
            # failure again => EOF
            if document.character(position) == None:
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
            print('insert spaces instead of tabulators')
            indentationCharacters.configurationUseTabs = False

        indentWidth = str(group.readEntry('Indentation Width'))
        indentationCharacters.configurationIndentWidth = int(indentWidth) if indentWidth else 4
    # indent with tabs or spaces
    useTabs = True
    spaceIndent = v.variable('space-indent')
    if spaceIndent == 'on':
        useTabs = False
    elif spaceIndent == 'off':
        useTabs = True
    else:
        useTabs = indentationCharacters.configurationUseTabs

    if useTabs:
        return '\t'
    else:
        indentWidth = v.variable('indent-width')
        if indentWidth and indentWidth.isdigit():
            return ' ' * int(indentWidth)
        else:
            # default
            return ' ' * indentationCharacters.configurationIndentWidth


@kate.action('Expand', shortcut='Ctrl+E', menu='Edit')
def expandAtCursor():
    """Attempt text expansion on the word at the cursor.
    The expansions available are based firstly on the mimetype of the
    document, for example "text_x-c++src.expand" for "text/x-c++src", and
    secondly on "all.expand".
    """
    document = kate.activeDocument()
    view = document.activeView()
    try:
        word_range, argument_range = wordAndArgumentAtCursorRanges(document, view.cursorPosition())
    except ParseError as e:
        kate.popup('Parse error:', e)
        return
    word = document.text(word_range)
    mime = str(document.mimeType())
    expansions = loadExpansions(mime)
    try:
        func = expansions[word]
    except KeyError:
        ui.popup('Error', 'Expansion "%s" not found :(' % word, 'dialog-warning')
        return
    arguments = []
    namedArgs = {}
    if argument_range is not None:
        # strip parentheses and split arguments by comma
        preArgs = [arg.strip() for arg in document.text(argument_range)[1:-1].split(',') if bool(arg.strip())]
        print('>> EXPAND: arguments = ' + repr(arguments))
        # form a dictionary from args w/ '=' character, leave others in a list
        for arg in preArgs:
            print('>> EXPAND: current arg = ' + repr(arg))
            if '=' in arg:
                key, value = [item.strip() for item in arg.split('=')]
                print('>> EXPAND: key = ' + repr(key))
                print('>> EXPAND: value = ' + repr(value))
                namedArgs[key] = value
            else:
                arguments.append(arg)
    # Call user expand function w/ parsed arguments and
    # possible w/ named params dict
    try:
        print('>> EXPAND: arguments = ' + repr(arguments))
        print('>> EXPAND: named arguments = ' + repr(namedArgs))
        if len(namedArgs):
            replacement = func(*arguments, **namedArgs)
        else:
            replacement = func(*arguments)
    except Exception as e:
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
                l.insert(0, 'Traceback (most recent call last):\n')
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
        ui.popup('Error', '<p style="white-space:pre">%s</p>' % s, 'dialog-error')
        return

    #KateDocumentConfig::cfReplaceTabsDyn
    indentCharacters = indentationCharacters(document)
    # convert newlines followed by tab characters to whatever spacing
    # the user... uses.
    for i in range(100):
        if '\n' + (indentCharacters * i) + '\t' in replacement:
            replacement = replacement.replace('\n' + (indentCharacters * i) + '\t', '\n' + (indentCharacters * (i + 1)))
    insertPosition = word_range.start()
    line = document.line(insertPosition.line())
    # autoindent: add the line's leading whitespace for each newline
    # in the expansion
    whitespace = ''
    for character in line:
        if character in ' \t':
            whitespace += character
        else:
            break
    replacement = replacement.replace('\n', '\n' + whitespace)
    # is desired cursor position set?
    cursorAdvancement = None
    if '%{cursor}' in replacement:
        cursorAdvancement = replacement.index('%{cursor}')
        # strip around that word
        replacement = replacement[:cursorAdvancement] + replacement[cursorAdvancement + 9:]
    # make the removal and insertion an atomic operation
    document.startEditing()
    if argument_range is not None:
        document.removeText(argument_range)
    document.removeText(word_range)
    document.insertText(insertPosition, replacement)
    # end before moving the cursor to avoid a crash
    document.endEditing()

    if cursorAdvancement is not None:
        # TODO The smartInterface isn't available anymore!
        #      But it's successor (movingInterface) isn't available yet in
        #      PyKDE4 <= 4.8.3 (at least) :( -- so, lets move a cursor manually...
        while True:
            currentLength = document.lineLength(insertPosition.line())
            if cursorAdvancement <= currentLength:
                break
            else:
                insertPosition.setLine(insertPosition.line() + 1)
                cursorAdvancement -= currentLength + 1      # NOTE +1 for every \n char
        insertPosition.setColumn(insertPosition.column() + cursorAdvancement)
        view.setCursorPosition(insertPosition)


# kate: space-indent on; indent-width 4;
