
''' Interactive console for inspecting Kate's internals and
playing about. The console provides syntax highlighting by
tokenizing the code using standard library module tokenize '''

import sys
from cStringIO import StringIO
import keyword
import tokenize
import token # for constants
import code

from PyQt4 import QtCore, QtGui

import kate

# @kate.init
# def foo():
    # print 'on kate.init: hello world'
    # window = kate.application.activeMainWindow()
    # w = window.window()
    # # print w

def tok(s):
    gen = tokenize.generate_tokens(StringIO(s).readline)
    l = []
    while True:
        try:
            token = gen.next()
        except tokenize.TokenError, e:
            l.append(e)
            break
        except StopIteration:
            break
        l.append(token)
    return l


class Console(code.InteractiveConsole):
    ''' The standard library module code doesn't provide you with
    string when you evaluate an expression (e.g "[]"); instead, it
    gets printed to stdout. Gross. This patches that problem but
    will probably not be massively forward compatible '''
    def runcode(self, c):
        stdout = sys.stdout
        sys.stdout = StringIO()
        try:
            exec c in self.locals
        except SystemExit:
            raise
        except:
            self.showtraceback()
        else:
            if code.softspace(sys.stdout, 0):
                print
        r = sys.stdout.getvalue()
        sys.stdout = stdout
        self.write(r)
    
    def write(self, s):
        sys.stdout.write(repr(s) + '\n')


class Exit:
    def __init__(self, window):
        self.window = window
    def __repr__(self):
        return 'Type exit() or Ctrl+D to exit.'
    def __str__(self):
        return repr(self)
    def __call__(self):
        w = self.window
        del self.window
        w.close()


class Helper:
    def __init__(self, console):
        self.console = console
    
    def __call__(self, o):
        s = self.console.state
        self.console.state = 'help'
        help(o)


class KateConsoleHighlighter(QtGui.QSyntaxHighlighter):
    ''' Pretty ruddy convulted, but still pretty awesome '''
    def __init__(self, console):
        self.console = console
        QtGui.QSyntaxHighlighter.__init__(self, console.document())
        self.keywordFormat = QtGui.QTextCharFormat()
        self.keywordFormat.setFontWeight(QtGui.QFont.Bold)
        for name, color in (
            ('prompt', QtGui.QColor(160, 160, 160)),
            ('string', QtGui.QColor(190, 3, 3)),
            ('name', QtGui.QColor('green')),
            ('integer', QtGui.QColor(0, 20, 255)),
            ('float', QtGui.QColor(176, 126, 0)),
            ('help', QtGui.QColor('green')),
            ('exception', QtGui.QColor(180, 3, 3)),
        ):
            format = QtGui.QTextCharFormat()
            format.setForeground(QtGui.QBrush(color))
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
        line = str(line)
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



class KateConsole(QtGui.QTextEdit):
    normalPrompt = '>>> '
    morePrompt = '    '
    def __init__(self, parent=None):
        QtGui.QTextEdit.__init__(self, parent)
        self.setWordWrapMode(QtGui.QTextOption.WrapAnywhere)
        font = QtGui.QFont('monospace')
        self.setFont(font)
        self.keyToMethod = {}
        # font = 
        for methodName in dir(self):
            if methodName.startswith('key'):
                key = getattr(QtCore.Qt, 'Key_' + methodName[3:], None)
                if key is None:
                    continue
                self.keyToMethod[key] = getattr(self, methodName)
        self.history = []
        self.historyPosition = 0
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
        self.console = Console(builtins)
        self.console.write = self.displayResult
        self.console.showtraceback = self.showTraceback
        self.state = 'normal'
        self.setPlainText(self.prompt)
        KateConsoleHighlighter(self)
        QtCore.QTimer.singleShot(0, self.moveCursorToEnd)
    
    def showTraceback(self):
        self.state = 'exception'
        code.InteractiveConsole.showtraceback(self.console)
    
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
        cursor.movePosition(QtGui.QTextCursor.End, QtGui.QTextCursor.MoveAnchor)
        newLine = cursor.blockNumber()
        if oldLine != newLine:
            self.setTextCursor(cursor)
    
    def keyPressEvent(self, e):
        key = e.key()
        # allow Ctrl+C
        if not (key == QtCore.Qt.Key_Control or e.matches(QtGui.QKeySequence.Copy)):
            self.moveCursorToEndIfNecessary()
        
        if key in self.keyToMethod:
            result = self.keyToMethod[key]()
            if result is True:
                QtGui.QTextEdit.keyPressEvent(self, e)
        else:
            #
            if e.modifiers() & QtCore.Qt.ControlModifier and e.key() == QtCore.Qt.Key_D:
                self.window().close()
            else:
                QtGui.QTextEdit.keyPressEvent(self, e)
    
    def moveCursorToEndOfLine(self):
        cursor = self.textCursor()
        cursor.movePosition(QtGui.QTextCursor.EndOfLine, QtGui.QTextCursor.MoveAnchor)
        self.setTextCursor(cursor)
    
    @property
    def prompt(self):
        if self.state == 'normal':
            return self.normalPrompt
        elif self.state == 'more':
            return self.morePrompt
    
    def moveCursorToStartOfLine(self):
        cursor = self.textCursor()
        cursor.movePosition(QtGui.QTextCursor.StartOfLine, QtGui.QTextCursor.MoveAnchor)
        cursor.movePosition(QtGui.QTextCursor.Right, QtGui.QTextCursor.MoveAnchor, len(self.prompt))
        self.setTextCursor(cursor)
    
    def moveCursorToEnd(self):
        cursor = self.textCursor()
        cursor.movePosition(QtGui.QTextCursor.End, QtGui.QTextCursor.MoveAnchor)
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
        # XX implement history
        pass
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


class KateConsoleDialog(QtGui.QDialog):
    def __init__(self, parent=None):
        QtGui.QDialog.__init__(self, parent)
        layout = QtGui.QVBoxLayout(self)
        layout.setMargin(0)
        layout.setSpacing(0)
        self.console = KateConsole(self)
        layout.addWidget(self.console)
        self.resize(600, 420)
    
    def closeEvent(self, e):
        # XX save size and position
        QtGui.QDialog.closeEvent(self, e)


@kate.action('Python Console', icon='utilities-terminal', shortcut='Ctrl+Shift+P', menu='View')
def showConsole():
    parent = kate.mainWindow()
    dialog = KateConsoleDialog(parent)
    dialog.show()


# Testing testing 1 2 3

#@init
#def foo():
#    while True:
#        code, success = QtGui.QInputDialog.getText(None, 'Line', 'Code:')
#        if not success:
#            break
#        success = success
#        code = str(code)
#        print '> %s' % code
#        try:
#            eval(code)
#        except SyntaxError:
#            exec code

