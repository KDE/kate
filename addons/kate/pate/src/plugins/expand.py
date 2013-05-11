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

import functools
import imp
import inspect
import os
import re
import sys
import time
import traceback

from PyQt4.QtGui import QToolTip

from PyKDE4.kdecore import KConfig, i18nc

import kate

from libkatepate import ui, common
from libkatepate.autocomplete import AbstractCodeCompletionModel


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
                raise ParseError(i18nc('@info', 'end of file reached'))
            else:
                if state in ('"', "'"):
                    raise ParseError(
                        i18nc(
                            '@info'
                          , 'end of line reached while searching for <icode>%1</icode>', state
                          )
                      )
    return position


def loadExpansionsFromFile(path):
    print('ExpandPlugin: loading expansions from {}'.format(path))
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
        # NOTE Detect ONLY a real function!
        if not name.startswith('__') and inspect.isfunction(o):
            expansions[o.__name__] = (o, path)
            print('ExpandPlugin: adding expansion `{}`'.format(o.__name__))
    return expansions


def mergeExpansions(left, right):
    assert(isinstance(left, dict) and isinstance(right, dict))
    result = left
    for exp_key, exp_tuple in right.items():
        if exp_key not in result:
            result[exp_key] = exp_tuple
        else:
            result[exp_key] = result[exp_key]
            print('ExpandPlugin: WARNING: Ignore duplicate expansion `{}` from {}'.format(exp_key, exp_tuple[1]))
            print('ExpandPlugin: WARNING: First defined here {}'.format(result[exp_key][1]))
    return result


def _getExpansionsFor(mime):
    expansions = {}
    mime_filename = mime.replace('/', '_') + '.expand'
    for directory in kate.applicationDirectories('expand'):
        if os.path.exists(os.path.join(directory, mime_filename)):
            expansions = mergeExpansions(
                expansions
              , loadExpansionsFromFile(os.path.join(directory, mime_filename))
              )
    return expansions


@functools.lru_cache()
def getExpansionsFor(mime):
    print('ExpandPlugin: collecting expansions for MIME {}'.format(mime))
    result = mergeExpansions(_getExpansionsFor(mime), _getExpansionsFor('all'))
    print('ExpandPlugin: got {} expansions at the end'.format(len(result)))
    return result


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


@kate.action(i18nc('@action:inmenu', 'Expand Usage'), shortcut='Shift+Ctrl+E')
def getHelpOnExpandAtCursor():
    document = kate.activeDocument()
    view = document.activeView()
    try:
        word_range, argument_range = wordAndArgumentAtCursorRanges(document, view.cursorPosition())
    except ParseError as e:
        ui.popup(i18nc('@title:window', 'Parse error'), e, 'dialog-warning')
        return
    word = document.text(word_range)
    expansions = getExpansionsFor(document.mimeType())
    if word in expansions:
        func = expansions[word][0]
        cursor_pos = view.cursorPositionCoordinates()
        tooltip_text = '\n'.join([
            line[8:] if line.startswith(' ' * 8) else line for line in func.__doc__.splitlines()
          ])
        print("Expand: help on {}: {}".format(word, tooltip_text))
        QToolTip.showText(cursor_pos, tooltip_text)
    else:
        print('ExpandPlugin: WARNING: undefined expansion `{}`'.format(word))


@kate.action(i18nc('@action:inmenu a verb', 'Expand'), shortcut='Ctrl+E', menu='Edit')
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
        ui.popup(i18nc('@title:window', 'Parse error'), e, 'dialog-warning')
        return
    word = document.text(word_range)
    expansions = getExpansionsFor(document.mimeType())
    if word in expansions:
        func = expansions[word][0]
    else:
        ui.popup(
            i18nc('@title:window', 'Error')
          , i18nc('@info:tooltip', 'Expansion "<icode>%1</icode>" not found', word)
          , 'dialog-warning'
          )
        return
    arguments = []
    namedArgs = {}
    if argument_range is not None:
        # strip parentheses and split arguments by comma
        preArgs = [arg.strip() for arg in document.text(argument_range)[1:-1].split(',') if bool(arg.strip())]
        print('ExpandPlugin: arguments = {}'.format(arguments))
        # form a dictionary from args w/ '=' character, leave others in a list
        for arg in preArgs:
            if '=' in arg:
                key, value = [item.strip() for item in arg.split('=')]
                namedArgs[key] = value
            else:
                arguments.append(arg)
    # Call user expand function w/ parsed arguments and
    # possible w/ named params dict
    try:
        print('ExpandPlugin: arguments = {}'.format(arguments))
        print('ExpandPlugin: named arguments = {}'.format(namedArgs))
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
                l.insert(0, i18nc('@info', 'Traceback (most recent call last):<nl/>'))
            l[len(l):] = traceback.format_exception_only(type, value)
        finally:
            tblist = tb = None
        # convert file names in the traceback to links. Nice.
        def replaceAbsolutePathWithLinkCallback(match):
            text = match.group()
            filePath = match.group(1)
            fileName = os.path.basename(filePath)
            text = text.replace(filePath, i18nc('@info', '<link url="%1">%2</link>', filePath, fileName))
            return text
        s = ''.join(l).strip()
        # FIXME: extract the filename and then use i18nc, instead of manipulate the i18nc text
        s = re.sub(
            i18nc('@info', 'File <filename>"(/[^\n]+)"</filename>, line')
          , replaceAbsolutePathWithLinkCallback
          , s
          )
        ui.popup(
            i18nc('@title:window', 'Error')
          , i18nc('@info:tooltip', '<bcode>%1</bcode>', s)
          , 'dialog-error'
          )
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



class ExpandsCompletionModel(AbstractCodeCompletionModel):
    TITLE_AUTOCOMPLETION = i18nc('@label:listbox', 'Expands Available')
    GROUP_POSITION = AbstractCodeCompletionModel.GroupPosition.GLOBAL

    def completionInvoked(self, view, word, invocationType):
        self.reset()
        # NOTE Do not allow automatic popup cuz most of expanders are short
        # and it will annoying when typing code...
        if invocationType == 0:
            return

        expansions = getExpansionsFor(view.document().mimeType())
        for exp, fn in expansions.items():
            # Try to get a function description (very first line)
            d = fn.__doc__
            if d != None:
                lines = d.splitlines()
                d = lines[0].strip()
            # Get function parameters
            fp = inspect.getargspec(fn)
            args = fp[0]
            params=''
            if len(args) != 0:
                params = ", ".join(args)
            if fp[1] != None:
                if len(params):
                    params += ', '
                params += '['+fp[1]+']'
            # Append to result completions list
            self.resultList.append(
                self.createItemAutoComplete(text=exp, description=d, args='('+params+')')
              )

    def reset(self):
        self.resultList = []


def _reset(*args, **kwargs):
    expands_completation_model.reset()


@kate.init
@kate.viewCreated
def createSignalAutocompleteExpands(view=None, *args, **kwargs):
    view = view or kate.activeView()
    if view:
        cci = view.codeCompletionInterface()
        cci.registerCompletionModel(expands_completation_model)


expands_completation_model = ExpandsCompletionModel(kate.application)
expands_completation_model.modelReset.connect(_reset)

# kate: space-indent on; indent-width 4;
