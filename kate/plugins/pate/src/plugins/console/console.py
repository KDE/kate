''' Interactive console for inspecting Kate's internals and playing about.

The console provides syntax highlighting by tokenizing the code using standard 
library module tokenize.'''
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
# Copyright (C) 2012 Shaheed Haque <srhaque@theiet.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) version 3.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

from __future__ import print_function
import code
from io import StringIO
import keyword
import os.path
import sys
import tokenize
import token # for constants

from PyQt4 import uic
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyKDE4.kdecore import *
from PyKDE4.kdeui import *

import kate

# @kate.init
# def foo():
    # print 'on kate.init: hello world'
    # window = kate.application.activeMainWindow()
    # w = window.window()
    # # print w

def tokold(s):
    gen = tokenize.generate_tokens(StringIO(s).readline)
    l = []
    while True:
        try:
            token = gen.next()
        except tokenize.TokenError as e:
            l.append(e)
            break
        except StopIteration:
            break
        l.append(token)
    return l

def tok(s):
    line = StringIO(s).readline
    l = []
    try:
        for token in tokenize.generate_tokens(line):
            l.append(token)
    except tokenize.TokenError:
        pass
    return l

class ConfigWidget(KTabWidget):
    """Configuration widget for this plugin."""
    #
    # Font.
    #
    font = None
    #
    # Primary and continuation prompts.
    #
    ps1 = None
    ps2 = None
    #
    # Highlight colours.
    #
    promptColour = None
    stringColour = None
    nameColour = None
    integerColour = None
    floatColour = None
    helpColour = None
    exceptionColour = None

    def __init__(self, parent = None):
        super(ConfigWidget, self).__init__(parent)

        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), "config.ui"), self)
        self.font.setFont(self.font.font(), True)
        self.font.enableColumn(KFontChooser.StyleList, False)

        self.reset();

    def apply(self):
        kate.configuration["font"] = self.font.font().toString()
        kate.configuration["ps1"] = self.ps1.text()
        kate.configuration["ps2"] = self.ps2.text()
        kate.configuration["promptColour"] = self.promptColour.color()
        kate.configuration["stringColour"] = self.stringColour.color()
        kate.configuration["nameColour"] = self.nameColour.color()
        kate.configuration["integerColour"] = self.integerColour.color()
        kate.configuration["floatColour"] = self.floatColour.color()
        kate.configuration["helpColour"] = self.helpColour.color()
        kate.configuration["exceptionColour"] = self.exceptionColour.color()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if "font" in kate.configuration:
            font = QFont()
            font.fromString(kate.configuration["font"])
            self.font.setFont(font, True)
        if "ps1" in kate.configuration:
            self.ps1.setText(kate.configuration["ps1"])
        if "ps2" in kate.configuration:
            self.ps2.setText(kate.configuration["ps2"])
        if "promptColour" in kate.configuration:
            self.promptColour.setColor(kate.configuration["promptColour"])
        if "stringColour" in kate.configuration:
            self.stringColour.setColor(kate.configuration["stringColour"])
        if "nameColour" in kate.configuration:
            self.nameColour.setColor(kate.configuration["nameColour"])
        if "integerColour" in kate.configuration:
            self.integerColour.setColor(kate.configuration["integerColour"])
        if "floatColour" in kate.configuration:
            self.floatColour.setColor(kate.configuration["floatColour"])
        if "helpColour" in kate.configuration:
            self.helpColour.setColor(kate.configuration["helpColour"])
        if "exceptionColour" in kate.configuration:
            self.exceptionColour.setColor(kate.configuration["exceptionColour"])

    def defaults(self):
        self.font.setFont(QFont('monospace'), True)
        self.ps1.setText(">>>")
        self.ps2.setText("...")
        self.promptColour.setColor(QColor(160, 160, 160))
        self.stringColour.setColor(QColor(190, 3, 3))
        self.nameColour.setColor(QColor('green'))
        self.integerColour.setColor(QColor(0, 20, 255))
        self.floatColour.setColor(QColor(176, 126, 0))
        self.helpColour.setColor(QColor('green'))
        self.exceptionColour.setColor(QColor(180, 3, 3))

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

def softspace(file, newvalue):
    oldvalue = 0
    try:
        oldvalue = file.softspace
    except AttributeError:
        pass
    try:
        file.softspace = newvalue
    except (AttributeError, TypeError):
        # "attribute-less object" or "read-only attributes"
        pass
    return oldvalue

class StringIOHack():

    def __init__(self):
        self.writeStrings = ""

    def close(self):
        self.writeStrings = None

    def write(self, s):
        self.writeStrings += s

    def getvalue(self):
        return self.writeStrings

class Console(code.InteractiveConsole):
    ''' The standard library module code doesn't provide you with
    string when you evaluate an expression (e.g "[]"); instead, it
    gets printed to stdout. Gross. This patches that problem but
    will probably not be massively forward compatible '''
    def __init__(self, writer, tracer, locals = None, filename = "<console>"):
        #super(Console, self).__init__(locals, filename)
        code.InteractiveConsole.__init__(self, locals, filename)
        self.write = writer
        self.tracer = tracer

    def runcode(self, c):
        stdout = sys.stdout
        sys.stdout = StringIOHack()
        try:
            exec(c, self.locals)
        except SystemExit:
            raise
        except:
            self.showtraceback()
        else:
            if softspace(sys.stdout, 0):
                print("")
        r = sys.stdout.getvalue()
        sys.stdout = stdout
        self.write(r)

    def showtraceback(self):
        self.tracer()
        stderr = sys.stderr
        sys.stderr = StringIOHack()
        #super(Console, self).showtraceback()
        code.InteractiveConsole.showtraceback(self)
        r = sys.stderr.getvalue()
        sys.stderr = stderr
        self.write(r)

class Exit:
    def __init__(self, window):
        self.window = window

    def __repr__(self):
        return i18n('Type exit() or Ctrl+D to exit.')

    def __str__(self):
        return repr(self)

    def __call__(self):
        w = self.window
        del self.window
        w.close()


class Helper:
    def __init__(self, console):
        self.console = console

    def __repr__(self):
        return i18n('Type help(object) for help on  object.\nType an expression to evaluate the expression.')

    def __str__(self):
        return repr(self)

    def __call__(self, o = None):
        if not o:
            print(self.__str__())
            return
        s = self.console.state
        self.console.state = 'help'
        help(o)

class History(KHistoryComboBox):

    def __init__(self, parent):
        super(History, self).__init__(parent)
        self.setMaxCount(50)
        self.resize(300, self.height())
        self.setCompletionMode(KGlobalSettings.CompletionPopupAuto)
        self.returnPressed[str].connect(self._doneCompletion)
        self.hide()

    @pyqtSlot("QString")
    def _doneCompletion(self, line):
        self.parent().console.displayResult(line)
        self.hide()

    def recall(self, left, top):
        self.move(left + 6, top - 6)
        self.show()
        self.setFocus(Qt.PopupFocusReason)

    def append(self, line):
        self.addToHistory(line)

class KateConsoleHighlighter(QSyntaxHighlighter):
    ''' Pretty ruddy convulted, but still pretty awesome '''
    def __init__(self, console):
        self.console = console
        QSyntaxHighlighter.__init__(self, console.document())
        self.keywordFormat = QTextCharFormat()
        self.keywordFormat.setFontWeight(QFont.Bold)
        for name in (
            'prompt',
            'string',
            'name',
            'integer',
            'float',
            'help',
            'exception'):
            format = QTextCharFormat()
            format.setForeground(QBrush(kate.configuration[name + "Colour"]))
            setattr(self, name + 'Format', format)

        self.tokenHandlers = {
            token.NAME: self.handleName,
            token.ERRORTOKEN: self.handleError,
            token.STRING: self.handleString,
            token.NUMBER: self.handleNumber,
        }
        self.overrideFormat = None
        self.singleMultiLineString = 1
        self.doubleMultiLineString = 2

    def highlightBlock(self, line):
        # print 'highlight %r' % line
        # print repr(prompt), repr(line), repr(self.console.state)
        if self.console.inputting:
            offset = 0
            prompt = self.console.prompt
            if line.startswith(prompt):
                self.setFormat(0, len(prompt.rstrip()), self.promptFormat)
                offset = len(prompt)
                line = line[offset:]
            if self.console.state == 'normal':
                state = -1
            else:
                state = self.previousBlockState()
            self.overrideFormat = None
            if state == self.singleMultiLineString:
                line = "'''" + line
                offset -= 3
            elif state == self.doubleMultiLineString:
                line = '"""' + line
                offset -= 3
                # print 'set'
            # print state, line
            # print 'offset:', offset
            # tokeniize
            tokens = tok(line)
            for token in tokens:
                if isinstance(token, tokenize.TokenError):
                    e = token
                    if e.args[0] == 'EOF in multi-line string':
                        tokens = tok(line + '"""')
                        if not isinstance(tokens[-1], tokenize.TokenError):
                            self.setCurrentBlockState(self.doubleMultiLineString)
                        else:
                            tokens = tok(line + "'''")
                            if not isinstance(tokens[-1], tokenize.TokenError):
                                self.setCurrentBlockState(self.singleMultiLineString)
                            else:
                                raise tokens[-1]
                        start = e.args[1][1]
                        self.setFormat(start + offset, len(line) - start, self.stringFormat)
                    else:
                        break
                else:
                    tokenType = token[0]
                    handler = self.tokenHandlers.get(tokenType)
                    start = token[2][1]
                    end = token[3][1]
                    if self.overrideFormat:
                        self.setFormat(offset + start, end - start, self.overrideFormat)
                        continue
                    if handler is not None:
                        format = handler(token)
                        if format is not None:
                            self.setFormat(offset + start, end - start, format)
                # print line, tok(line)
        elif self.console.helping:
            self.setFormat(0, len(line), self.helpFormat)
        elif self.console.excepting:
            self.setFormat(0, len(line), self.exceptionFormat)
        else:
            # print 'unknown state:', self.console.state
            pass

    def handleName(self, token):
        name = token[1]
        if keyword.iskeyword(name):
            return self.keywordFormat
        elif name in ('True', 'False', 'None', 'self', 'cls'):
            return self.nameFormat

    def handleString(self, token):
        return self.stringFormat

    def handleError(self, token):
        value = token[1]
        if value in ('"', "'"):
            self.overrideFormat = self.stringFormat

    def handleNumber(self, token):
        number = token[1]
        if '.' in number or 'e' in number:
            return self.floatFormat
        return self.integerFormat



class KateConsole(QTextEdit):
    def __init__(self, parent=None):
        QTextEdit.__init__(self, parent)
        self.setWordWrapMode(QTextOption.WrapAnywhere)
        font = QFont()
        font.fromString(kate.configuration["font"])
        self.setFont(font)
        self.keyToMethod = {}
        # font =
        for methodName in dir(self):
            if methodName.startswith('key'):
                key = getattr(Qt, 'Key_' + methodName[3:], None)
                if key is None:
                    continue
                self.keyToMethod[key] = getattr(self, methodName)
        self.history = History(self.window())
        self.buffer = ''
        exit = Exit(self.window())
        builtins = {
            'kate': kate,
            'KTextEditor': kate.KTextEditor,
            'Kate': kate.Kate,
            'exit': exit,
            'quit': exit,
            'help': Helper(self),
            '__name__': __name__,
        }
        self.console = Console(self.displayResult, self.showTraceback, builtins)
        self.state = 'normal'
        self.setPlainText(self.prompt)
        KateConsoleHighlighter(self)
        QTimer.singleShot(0, self.moveCursorToEnd)

    def showTraceback(self):
        self.state = 'exception'

    @property
    def inputting(self):
        return self.state in ('normal', 'more')

    @property
    def helping(self):
        return self.state == 'help'

    @property
    def excepting(self):
        return self.state == 'exception'

    def moveCursorToEndIfNecessary(self):
        cursor = self.textCursor()
        oldLine = cursor.blockNumber()
        cursor.movePosition(QTextCursor.End, QTextCursor.MoveAnchor)
        newLine = cursor.blockNumber()
        if oldLine != newLine:
            self.setTextCursor(cursor)

    def keyPressEvent(self, e):
        key = e.key()
        # allow Ctrl+C
        if not (key == Qt.Key_Control or e.matches(QKeySequence.Copy)):
            self.moveCursorToEndIfNecessary()

        if key in self.keyToMethod:
            result = self.keyToMethod[key]()
            if result is True:
                QTextEdit.keyPressEvent(self, e)
        else:
            #
            if e.modifiers() & Qt.ControlModifier and e.key() == Qt.Key_D:
                self.window().close()
            else:
                QTextEdit.keyPressEvent(self, e)

    def moveCursorToEndOfLine(self):
        cursor = self.textCursor()
        cursor.movePosition(QTextCursor.EndOfLine, QTextCursor.MoveAnchor)
        self.setTextCursor(cursor)

    @property
    def prompt(self):
        if self.state == 'normal':
            return kate.configuration["ps1"]
        elif self.state == 'more':
            return kate.configuration["ps2"]

    def moveCursorToStartOfLine(self):
        cursor = self.textCursor()
        cursor.movePosition(QTextCursor.StartOfLine, QTextCursor.MoveAnchor)
        cursor.movePosition(QTextCursor.Right, QTextCursor.MoveAnchor, len(self.prompt))
        self.setTextCursor(cursor)

    def moveCursorToEnd(self):
        cursor = self.textCursor()
        cursor.movePosition(QTextCursor.End, QTextCursor.MoveAnchor)
        self.setTextCursor(cursor)

    @property
    def line(self):
        line = str(self.textCursor().block().text()).rstrip()
        return line[len(self.prompt):]

    @property
    def columnNumber(self):
        return self.textCursor().columnNumber()
    @property
    def lineNumber(self):
        return self.textCursor().blockNumber()

    def displayResult(self, r):
        self.insertPlainText(r)

    def keyReturn(self):
        line = self.line
        self.append('')
        #
        # Add the line to the history. Multi-line input is added line-by-line
        # so each line is independently recall-able.
        #
        self.history.append(line)
        self.state = 'unknown'
        self.moveCursorToEnd()
        more = self.console.push(line)
        if more:
            self.state = 'more'
        else:
            self.state = 'normal'
        self.insertPlainText(self.prompt)
        self.moveCursorToEnd()

    def keyEnter(self):
        return self.keyReturn()

    def keyLeft(self):
        c = self.columnNumber
        if c < len(self.prompt):
            # something has gone wrong
            self.moveCursorToStartOfLine()
        elif c > len(self.prompt):
            return True

    def keyUp(self):
        self.history.recall(self.cursorRect().right(), self.cursorRect().top())

    def keyDown(self):
        pass

    def keyPageUp(self):
        pass
    def keyPageDown(self):
        pass

    def keyBackspace(self):
        c = self.columnNumber
        return c > len(self.prompt)

    def keyHome(self):
        self.moveCursorToStartOfLine()

    def keyEnd(self):
        self.moveCursorToEndOfLine()


class KateConsoleDialog(QDialog):
    def __init__(self, parent=None):
        QDialog.__init__(self, parent)
        layout = QVBoxLayout(self)
        layout.setMargin(0)
        layout.setSpacing(0)
        self.console = KateConsole(self)
        layout.addWidget(self.console)
        try:
            self.resize(kate.configuration["dialogSize"])
        except KeyError:
            self.resize(600, 420)

    def closeEvent(self, e):
        kate.configuration["dialogSize"] = self.size();
        QDialog.closeEvent(self, e)

@kate.action('Python Console', icon='utilities-terminal', shortcut='Ctrl+Shift+P', menu='View')
def showConsole():
    # Make all our config is initialised.
    ConfigWidget().apply()
    parent = kate.mainWindow()
    dialog = KateConsoleDialog(parent)
    dialog.show()

@kate.configPage("Python Console", "Python Console", icon = "utilities-terminal")
def configPage(parent = None, name = None):
    return ConfigPage(parent, name)


# Testing testing 1 2 3

#@init
#def foo():
#    while True:
#        code, success = QInputDialog.getText(None, 'Line', 'Code:')
#        if not success:
#            break
#        success = success
#        code = str(code)
#        print '> %s' % code
#        try:
#            eval(code)
#        except SyntaxError:
#            exec code

