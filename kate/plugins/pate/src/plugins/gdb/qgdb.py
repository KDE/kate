#
# Copyright 2009, 2012, Shaheed Haque <srhaque@theiet.org>.
#
# This is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this code. If not, see <http://www.gnu.org/licenses/>.
#

from __future__ import print_function
import curses.ascii
import logging
import os

from PyQt4.QtCore import *
from PyKDE4.kdecore import *
from IPython.zmq.ipkernel import IPKernelApp

from miparser import MiParser

class QGdbException(Exception):
    pass

class QGdbTimeoutError(QGdbException):
    pass

class QGdbInvalidResults(QGdbException):
    pass

class QGdbExecuteError(QGdbException):
    pass

class DebuggerKernel():
    """Start or stop the IPython "kernel" inside GDB's Python support.

    This kernel communicates via 0MQ to the remote IPython "shell"
    """
    _app = None

    def __init__(self):
        self._app = IPKernelApp.instance()
        self._app.initialize()
        fh = logging.FileHandler(self.__class__.__name__ + ".log")
        fh.setLevel(logging.DEBUG)
        log = self._app.kernel.log
        log.setLevel(logging.DEBUG)
        log.addHandler(fh)
        log.debug("kernel init")

    def start(self):
        self._app.kernel.log.debug("kernel start")
        self._app.start()

    def stop(self):
        self._app.kernel.log.debug("kernel stop")
        #self._app.shell.keepkernel_on_exit = True
        self._app.shell.ask_exit()
        self._app.kernel.log.debug("kernel stopping")
        print('shell being stopped is', self._app.shell)
        app = self._app
        app.exit_now = True
        if app.kernel.control_stream:
            self._app.kernel.log.debug("control_stream stop")
            app.kernel.control_stream.stop()
        for s in app.kernel.shell_streams:
            self._app.kernel.log.debug("shell_stream stop")
            s.stop_on_send()
            s.stop_on_recv()
            s.close()


class InferiorIo(QThread):
    _pty = None
    def __init__(self, parent):
        super(InferiorIo, self).__init__(parent)
        # TODO: convert to KPtyDevice
        self._pty = KPty()
        if not self._pty.open():
            raise QGdbException("Cannot open pty")
        #if not self._pty.openSlave():
        #    raise QGdbException("Cannot open pty slave")
        self._masterFd = os.fdopen(self._pty.masterFd())
        self._slaveFd = self._pty.slaveFd()
        self.start()

    def __del__(self):
        self._pty.closeSlave()
        os.close(self._masterFd)

    def run(self):
        while not (self.parent()._interruptPending):
            line = self._masterFd.readline()
            if not line:
                break
            self.parent().gdbStreamTarget.emit(line[:-1])
        print("inferior reader done!!!!")

    def interruptWait(self):
        """Interrupt an in-progress wait for response from GDB."""
        self._interruptPending = True

    def ttyName(self):
        return self._pty.ttyName()

class DebuggerIo(QThread):
    """A procedural interface to GDB running as a subprocess in a thread.
    A second thread is used to handle I/O with a direct inferior if needed.
    """
    _gdbThread = None
    _inferiorThread = None
    _gdbThreadStarted = None
    _interruptPending = None
    arguments = None
    _miToken = None
    def __init__(self, gdbThreadStarted, arguments, verbose = 0):
        """Constructor.

        @param _gdbThreadStarted    Signal completion via semaphore.
        @param arguments        GDB start command.
        """
        super(DebuggerIo, self).__init__()
        self.miParser = MiParser()
        self._gdbThreadStarted = gdbThreadStarted
        self.arguments = arguments
        self._miToken = 0

    def run(self):
        try:
            #self._gdbThread = pygdb.Gdb(GDB_CMDLINE, handler = self, verbose = verbose)
            self._gdbThread = QProcess()
            self._gdbThread.setProcessChannelMode(QProcess.MergedChannels)
            self._gdbThread.error.connect(self.gdbProcessError)
            self._gdbThread.finished.connect(self.gdbProcessFinished)
            self._gdbThread.readyReadStandardOutput.connect(self.gdbProcessReadyReadStandardOutput)
            self._gdbThread.started.connect(self.gdbProcessStarted)
            self._gdbThread.stateChanged.connect(self.gdbProcessStateChanged)
            #
            # Start.
            #
            self._gdbThread.start(self.arguments[0], self.arguments[1:])
            self._gdbThread.waitForStarted()
            self.waitForPromptConsole("cmd: " + self.arguments[0])
        except QGdbException as e:
            self.dbg0("TODO make signal work: {}", e)
            traceback.print_exc()
            self.dbg.emit(0, str(e))
        self._gdbThreadStarted.release()

    def interruptWait(self):
        """Interrupt an in-progress wait for response from GDB."""
        self._interruptPending = True

    def startIoThread(self):
        self._inferiorThread = InferiorIo(self)
        return self._inferiorThread.ttyName()

    def consoleCommand(self, command):
        self.dbg1("consoleCommand: {}", command)
        self._gdbThread.write(command + "\n")
        self._gdbThread.waitForBytesWritten()
        return self.waitForPromptConsole(command)

    def miCommand(self, command):
        """Execute a command using the GDB/MI interpreter."""
        self._miToken += 1
        command = "interpreter-exec mi \"{}{}\"".format(self._miToken, command)
        self.dbg1("miCommand: '{}'", command)
        self._gdbThread.write(command + "\n")
        self._gdbThread.waitForBytesWritten()
        return self.waitForPromptMi(self._miToken, command)

    def miCommandOne(self, command):
        """A specialisation of miCommand() where we expect exactly one result record."""
        error, records = self.miCommand(command)
        #
        # We expect exactly one record.
        #
        if error == curses.ascii.CAN:
            raise QGdbTimeoutError("Timeout after {} results, '{}' ".format(len(records), records))
        elif len(records) != 1:
            raise QGdbInvalidResults("Expected 1 result, not {}, '{}' ".format(len(records), records))
        status, results = records[0]
        self.dbg2("miCommandOne: {}", records[0])
        if status == "error":
            raise QGdbExecuteError(results[0][5:-1])
        return results

    def miCommandExec(self, command, args):
        self.miCommandOne(command)

    def waitForPromptConsole(self, why, endLine = None, timeoutMs = 10000):
        """Read responses from GDB until a prompt, or interrupt.

        @return (error, lines)    Where error is None (normal prompt seen),
                    curses.ascii.ESC (user interrupt) or
                    curses.ascii.CAN (caller timeout)
        """
        prompt = "(gdb) "

        lines = []
        maxTimeouts = timeoutMs / 100
        self.dbg1("reading for: {}", why)
        self._interruptPending = False
        while True:
            while not self._gdbThread.canReadLine()  and \
                    self._gdbThread.peek(len(prompt)) != prompt and \
                    not self._interruptPending and \
                    maxTimeouts:
                self._gdbThread.waitForReadyRead(100)
                maxTimeouts -= 1
            if self._gdbThread.canReadLine():
                line = self._gdbThread.readLine()
                line = unicode(line[:-1], "utf-8")
                lines.append(line)
                if endLine and line.startswith(endLine):
                    #
                    # Yay, got to the end!
                    #
                    self.dbg2("All lines read: {}", len(lines))
                    return (None, lines)
                #
                # We managed to read a line, so reset the timeout.
                #
                maxTimeouts = timeoutMs / 100
            elif self._gdbThread.peek(len(prompt)) == prompt:
                self._gdbThread.read(len(prompt))
                #
                # Yay, got to the end!
                #
                self.dbg2("All lines read: {}", len(lines))
                return (None, lines)
            elif self._interruptPending:
                #
                # User got fed up. Note, there may be more to read!
                #
                self.dbg0("Interrupt after {} lines read, '{}'", len(lines), lines)
                return (curses.ascii.ESC, lines)
            elif not maxTimeouts:
                #
                # Caller got fed up. Note, there may be more to read!
                #
                self.dbg0("Timeout after {} lines read, '{}'", len(lines), lines)
                return (curses.ascii.CAN, lines)

    def waitForPromptMi(self, token, why, timeoutMs = 10000):
        """Read responses from GDB until a prompt, or interrupt.

        @return (error, lines)    Where error is None (normal prompt seen),
                    curses.ascii.ESC (user interrupt) or
                    curses.ascii.CAN (caller timeout)
        """
        prompt = "(gdb) "
        lines = []
        maxTimeouts = timeoutMs / 100
        self.dbg1("reading for: {}", why)
        self._interruptPending = False
        token = str(token)
        while True:
            while not self._gdbThread.canReadLine() and \
                    self._gdbThread.peek(len(prompt)) != prompt and \
                    not self._interruptPending and \
                    maxTimeouts:
                self._gdbThread.waitForReadyRead(100)
                maxTimeouts -= 1
            if self._gdbThread.canReadLine():
                line = self._gdbThread.readLine()
                line = unicode(line[:-1], "utf-8")
                if line[0] == "~":
                    line = line[1:]
                    #
                    # Console stream record. Not added to return value!
                    #
                    self.gdbStreamConsole.emit(line)
                elif line[0] == "@":
                    line = line[1:]
                    #
                    # Target stream record. Not added to return value!
                    #
                    self.gdbStreamTarget.emit(line)
                elif line[0] == "&":
                    line = line[1:]
                    #
                    # Log stream record. Not added to return value!
                    #
                    self.gdbStreamLog.emit(line)
                elif line[0] == "*":
                    #
                    # OOB record.
                    #
                    line = line[1:]
                    tuple = self.parseOobRecord(line)
                    self.signalEvent(tuple[0], tuple[1])
                    lines.append(tuple)
                elif line.startswith(token + "^"):
                    #
                    # Result record.
                    #
                    line = line[len(token) + 1:]
                    tuple = self.parseResultRecord(line)
                    self.signalEvent(tuple[0], tuple[1])
                    lines.append(tuple)
                else:
                    # TODO: other record types.
                    self.dbg0("NYI: unexpected record string {}", line)
                #
                # We managed to read a line, so reset the timeout.
                #
                maxTimeouts = timeoutMs / 100
            elif self._gdbThread.peek(len(prompt)) == prompt:
                self._gdbThread.read(len(prompt))
                #
                # Yay, got to the end!
                #
                self.dbg2("All lines read: {}", len(lines))
                return (None, lines)
            elif self._interruptPending:
                #
                # User got fed up. Note, there may be more to read!
                #
                self.dbg0("Interrupt after {} lines read", len(lines))
                return (curses.ascii.ESC, lines)
            elif not maxTimeouts:
                #
                # Caller got fed up. Note, there may be more to read!
                #
                self.dbg0("Timeout after {} lines read", len(lines))
                return (curses.ascii.CAN, lines)

    def parseOobRecord(self, line):
        """GDB/MI OOB record."""
        self.dbg1("OOB string {}", line)
        tuple = line.split(",", 1)
        if tuple[0] == "stop":
            tuple[0] = ""
        else:
            self.dbg0("Unexpected OOB string {}", line)
        if len(tuple) > 1:
            tuple[1] = self.miParser.parse(tuple[1])
        else:
            tuple.append({})
        self.dbg1("OOB record {}", tuple)
        return tuple

    def parseResultRecord(self, line):
        """GDB/MI Result record.

        @param result    "error" for ^error
                "exit" for ^exit
                "" for normal cases (^done, ^running, ^connected)
        @param data    "c-string" for ^error
                "results" for ^done
        """
        self.dbg1("Result string {}", line)
        tuple = line.split(",", 1)
        if tuple[0] in ["done", "running" ]:
            tuple[0] = ""
        elif tuple[0] != "error":
            self.dbg0("Unexpected result string {}", line)
        if len(tuple) > 1:
            tuple[1] = self.miParser.parse(tuple[1])
        else:
            tuple.append({})
        self.dbg1("Result record {}", tuple)
        return tuple

    def signalEvent(self, event, args):
        """Signal whoever is interested of interesting events."""
        if event == "stop":
            self.onStopped.emit(args)
        elif event.startswith("thread-group"):
            if event == "thread-group-added":
                self.onThreadGroupAdded.emit(args)
            elif event == "thread-group-removed":
                self.onThreadGroupRemoved.emit(args)
            elif event == "thread-group-started":
                self.onThreadGroupStarted.emit(args)
            elif event == "thread-group-exited":
                self.onThreadGroupExited.emit(args)
            else:
                self.onUnknownEvent.emit(event, args)
        elif event.startswith("thread"):
            if event == "thread-created":
                self.onThreadCreated.emit(args)
            elif event == "thread-exited":
                self.onThreadExited.emit(args)
            elif event == "thread-selected":
                self.onThreadSelected.emit(args)
            else:
                self.onUnknownEvent.emit(event, args)
        elif event.startswith("library"):
            if event == "library-loaded":
                self.onLibraryLoaded.emit(args)
            elif event == "library-unloaded":
                self.onLibraryUnloaded.emit(args)
            else:
                self.onUnknownEvent.emit(event, args)
        elif event.startswith("breakpoint"):
            if event == "breakpoint-created":
                self.onBreakpointCreated.emit(args)
            elif event == "breakpoint-modified":
                self.onBreakpointModified.emit(args)
            elif event == "breakpoint-deleted":
                self.onBreakpointDeleted.emit(args)
            else:
                self.onUnknownEvent.emit(event, args)
        else:
            self.onUnknownEvent.emit(event, args)

    def dbg0(self, msg, *args):
        print("ERR-0", msg.format(*args))

    def dbg1(self, msg, *args):
        print("DBG-1", msg.format(*args))

    def dbg2(self, msg, *args):
        print("DBG-2", msg.format(*args))

    @pyqtSlot(QProcess.ProcessError)
    def gdbProcessError(self, error):
        self.dbg0("gdbProcessError: {}", error)

    @pyqtSlot(int, QProcess.ExitStatus)
    def gdbProcessFinished(self, exitCode, exitStatus):
        self.dbg2("gdbProcessFinished: {}, {}", exitCode, exitStatus)

    @pyqtSlot()
    def gdbProcessReadyReadStandardOutput(self):
        self.dbg2("gdbProcessReadyReadStandardOutput")

    @pyqtSlot()
    def gdbProcessStarted(self):
        self.dbg2("gdbProcessStarted")

    @pyqtSlot(QProcess.ExitStatus)
    def gdbProcessStateChanged(self, newState):
        self.dbg2("gdbProcessStateChanged: {}", newState)

    """GDB/MI Stream record, GDB console output."""
    gdbStreamConsole = pyqtSignal('QString')

    """GDB/MI Stream record, GDB target output."""
    gdbStreamTarget = pyqtSignal('QString')

    """GDB/MI Stream record, GDB log output."""
    gdbStreamLog = pyqtSignal('QString')

    onUnknownEvent = pyqtSignal('QString', dict)

    onRunning = pyqtSignal('QString')
    onStopped = pyqtSignal('QString', 'QString', 'QString', 'QString')

    """thread-group-added,id="id". """
    onThreadGroupAdded = pyqtSignal('QString')
    """thread-group-removed,id="id". """
    onThreadGroupRemoved = pyqtSignal('QString')
    """thread-group-started,id="id",pid="pid". """
    onThreadGroupStarted = pyqtSignal('QString', 'QString')
    """thread-group-exited,id="id"[,exit-code="code"]. """
    onThreadGroupExited = pyqtSignal('QString', 'QString')

    """thread-created,id="id",group-id="gid". """
    onThreadCreated = pyqtSignal('QString', 'QString')
    """thread-exited,id="id",group-id="gid". """
    onThreadExited = pyqtSignal('QString', 'QString')
    """thread-selected,id="id". """
    onThreadSelected = pyqtSignal('QString')

    """library-loaded,id="id",target-name,host-name,symbols-loaded[,thread-group].
    Note: symbols-loaded is not used"""
    onLibraryLoaded = pyqtSignal('QString', 'QString', 'QString', 'bool', 'QString')
    """library-unloaded,id="id",target-name,host-name[,thread-group]. """
    onLibraryUnloaded = pyqtSignal('QString', 'QString', 'QString', 'QString')

    """breakpoint-created,bkpt={...}. """
    onBreakpointCreated = pyqtSignal('QString')
    """breakpoint-modified,bkpt={...}. """
    onBreakpointModified = pyqtSignal('QString')
    """breakpoint-deleted,bkpt={...}. """
    onBreakpointDeleted = pyqtSignal('QString')

class Breakpoints():
    """Model of GDB breakpoints, tracepoints and watchpoints."""

    _gdb = None

    def __init__(self, gdb):
        """Constructor."""
        self._gdb = gdb

    def _breakInsert(self, trace, location, temporary, hw, disabled, condition, after, tid):
        """Create breakpoint or tracepoint.

        bkpt={number="number",type="type",disp="del"|"keep",
        enabled="y"|"n",addr="hex",func="funcname",file="filename",
        fullname="full_filename",line="lineno",[thread="threadno,]
        times="times"}
        """
        options = "-f"
        if temporary:
            options += " -t"
        if hw:
            options += " -h"
        if disabled:
            options += " -d"
        if trace:
            options += " -a"
        if condition:
            options += " -c " + condition
        if after:
            options += " -i " + after
        if tid:
            options += " -p " + tid
        if location:
            options += " " + location
        results = self._gdb.miCommandOne("-break-insert {}".format(options))
        return results["bkpt"]

    def breakpointCreate(self, location, temporary = False, hw = False, disabled = False, condition = None, after = 0, tid = None, probe = None):
        """Create breakpoint."""
        return self._breakInsert(False, location, temporary, hw, disabled, condition, after, tid)

    def tracepointCreate(self, location, temporary = False, hw = False, disabled = False, condition = None, after = 0, tid = None, probe = None):
        """Create tracepoint."""
        return self._breakInsert(True, location, temporary, hw, disabled, condition, after, tid)

    def watchpointCreate(self, expr, onWrite = True, onRead = False):
        """Create watchpoint."""
        options = ""
        if onWrite and onRead:
            options += " -a"
        elif onRead:
            options += " -r"
        elif not onWrite:
            raise QGdbException("Invalid watchpoint {}, {}".format(onWrite, onRead))
        results = self._gdb.miCommandOne("-break-watch {} {}".format(options, expr))
        return results["wpt"]

    def breakpointDelete(self, id):
        results = self._gdb.miCommandOne("-break-delete {}".format(id))
        return results

    def tracepointDelete(self, id):
        return self.breakpointDelete(id)

    def setAfter(self, id, count):
        # TODO
        #if id is watchpoint:
        #    results = self._gdb.miCommandOne("-break-passcount {} {}".format(id, count))
        #else:
        results = self._gdb.miCommandOne("-break-after {} {}".format(id, count))
        return results

    def setCommands(self, id, commands):
        commands = " ".join(["\"{}\"".format(c) for c in commands])
        results = self._gdb.miCommandOne("-break-commands {} {}".format(id, commands))
        return results

    def setCondition(self, id, condition):
        results = self._gdb.miCommandOne("-break-condition {} {}".format(id, condition))
        return results

    def setEnable(self, id, enabled):
        if enabled:
            results = self._gdb.miCommandOne("-break-enable {}".format(id))
        else:
            results = self._gdb.miCommandOne("-break-disable {}".format(id))
        return results

    def list(self, id):
        """
        [{u'bkpt': {u'disp': u'keep', u'addr': u'<MULTIPLE>', u'type': u'breakpoint', u'enabled': u'y', u'number': '2', u'original-location': u'QWidget::QWidget', u'times': '0'}}, {u'at': u'<QWidget::QWidget(QWidgetPrivate&, QWidget*, QFlags<Qt::WindowType>)>', u'enabled': u'y', u'number': u'2.1', u'addr': '0x7ffff6d601b0'}, {u'at': u'<QWidget::QWidget(QWidget*, char const*, QFlags<Qt::WindowType>)>', u'enabled': u'y', u'number': u'2.2', u'addr': '0x7ffff6d60270'}, {u'at': u'<QWidget::QWidget(QWidget*, QFlags<Qt::WindowType>)>', u'enabled': u'y', u'number': u'2.3', u'addr': '0x7ffff6d603c0'}]
        """
        if id:
            results = self._gdb.miCommandOne("-break-info {}".format(id))
        else:
            results = self._gdb.miCommandOne("-break-info")
        results = results[u'BreakpointTable'][u'body']
        if id:
            results = [row for row in results if row["number"] == id]
        return results

class Data():
    """Model of GDB data."""

    _gdb = None
    _registerNames = None
    _registerNumbers = None

    #
    # Paged memory reading.
    #
    _nextStart = None
    # Disassembly settings
    _nextEnd = None
    _filename = None
    _nextLine = None
    _lines = None
    # Memory dump settings.
    _wordFormat = None
    _wordSize = None
    _rows = None
    _columns = None
    _offsetBytes = None
    _asChar = None

    def __init__(self, gdb):
        """Constructor."""
        self._gdb = gdb

    def _getRegisterNames(self):
        """Map register numbers <-> names.

        Create mappings between register number and name, omitting the
        entries with null names. Note that we don't call this in the
        constructor because GDB's notion of the registers can change
        once the program is running.
        """
        if not self._registerNames:
            self._registerNames = dict()
            self._registerNumbers = dict()
            results = self._gdb.miCommandOne("-data-list-register-names")
            names = results[u'register-names']
            for i in range(len(names)):
                if len(names[i]):
                    self._registerNames[names[i]] = i
                    self._registerNumbers[i] = names[i]

    def listRegisterValues(self, fmt = 'r', regName = None):
        """
        [{u'number': '0', u'value': '0x7ffff77b0d90', 'name': u'rax'}, {u'number': '1', u'value': '0x6d6c20', 'name': u'rbx'}, {u'number': '2', u'value': '0x7fffffffd370', 'name': u'rcx'}, {u'number': '3', u'value': '0x0', 'name': u'rdx'}, {u'number': '4', u'value': '0x6d2f00', 'name': u'rsi'}]
        """
        self._getRegisterNames()
        if regName:
            try:
                regNo = self._registerNames[regName]
            except:
                raise QGdbException("Invalid register name {}".format(regName))
            results = self._gdb.miCommandOne("-data-list-register-values {} {}".format(fmt, regNo))
        else:
            results = self._gdb.miCommandOne("-data-list-register-values {}".format(fmt))
        results = results[u'register-values']
        for u in results:
            u["name"] = self._registerNumbers[int(u[u'number'])]
        return results

    def disassemble(self, mode, start_addr = _nextStart, end_addr = _nextEnd, filename = _filename, linenum = _nextLine, lines = _lines):
        """
        [{u'inst': u'push   %rax', u'address': '0x7fffffffdc10'}, {u'inst': u'cmpl   $0x7ffff6,(%rsi)', u'address': '0x7fffffffdc11'}]
        """
        options = ""
        if start_addr:
            options += " -s " + str(start_addr)
        if end_addr:
            options += " -e " + str(end_addr)
        if filename:
            options += " -f " + filename
        if linenum:
            options += " -l " + str(linenum)
        if lines:
            options += " -n " + str(lines)
        result = self._gdb.miCommandOne("-data-disassemble {} -- {}".format(options, mode))
        return result[u'asm_insns']

    def evalute(self, expr):
        return self._gdb.miCommandOne('-data-evaluate-expression', expr)

    def data_list_changed_registers(self):
        return self._gdb.miCommandOne('-data-list-changed-registers')

    def readMemory(self, address = _nextStart, word_format = _wordFormat, word_size = _wordSize, nr_rows = _rows, nr_cols = _columns, offset_bytes = _offsetBytes, aschar = _asChar):
        """
        [{u'data': ['0xf63e8150', '0x7fff'], u'addr': '0x7fffffffdc10'}, {u'data': ['0x645c10', '0x0'], u'addr': '0x7fffffffdc18'}, {u'data': ['0x69b220', '0x0'], u'addr': '0x7fffffffdc20'}, {u'data': ['0x0', '0x0'], u'addr': '0x7fffffffdc28'}]
        """
        if not address or not word_format or not word_size or not nr_rows or not nr_cols:
            raise QGdbException("Bad memory args")
        options = ""
        if offset_bytes:
            options += "-o " + offset_bytes
        results = self._gdb.miCommandOne("-data-read-memory {} {} {} {} {} {}".format(options, address, word_format, word_size, nr_rows, nr_cols))
        self._nextStart = results[u'next-page']
        self._wordFormat = word_format
        self._wordSize = word_size
        self._rows = nr_rows
        self._columns = nr_cols
        self._offsetBytes = offset_bytes
        self._asChar = aschar
        bytesRequested = results[u'total-bytes']
        bytesRead = results[u'nr-bytes']
        return results[u'memory']

class ProgramControl():
    """Model of GDB files."""

    _gdb = None

    def __init__(self, gdb):
        """Constructor."""
        self._gdb = gdb

    def allSources(self):
        """
        [{u'fullname': u'/home/srhaque/kdebuild/kate/kate/app/kate_dummy.cpp', u'file': u'/home/srhaque/kdebuild/kate/kate/app/kate_dummy.cpp'}, {u'fullname': u'/usr/include/c++/4.7/iostream', u'file': u'/usr/include/c++/4.7/iostream'}, ..., {u'fullname': u'/home/srhaque/kdebuild/kate/kate/app/kateinterfaces_automoc.cpp', u'file': u'/home/srhaque/kdebuild/kate/kate/app/kateinterfaces_automoc.cpp'}]
        """
        results = self._gdb.miCommandOne("-file-list-exec-source-files")
        return results[u'files']

    def currentSource(self):
        """
        {u'line': '3', u'fullname': u'/home/srhaque/kdebuild/kate/kate/app/kate_dummy.cpp', u'file': u'/home/srhaque/kdebuild/kate/kate/app/kate_dummy.cpp', u'macro-info': '0'}
        """
        results = self._gdb.miCommandOne("-file-list-exec-source-file")
        return results

    def execSections(self):
        results = self._gdb.miCommandOne("-file-list-exec-sections")
        #
        # [(u'variables', [{u'name': u'options', u'value': u'{d = 0x622320}'}, {u'type': u'KateApp * const', u'name': u'this', u'value': '0x7fffffffdc10', u'arg': '1'}, {u'type': u'KCmdLineArgs *', u'name': u'args', u'value': '0x6237f0', u'arg': '1'}])]
        #
        u = results[0][1]
        print("#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], u["from"]))

    def sharedLibraries(self):
        results = self._gdb.miCommandOne("-file-list-shared-libraries")
        #
        # [(u'variables', [{u'name': u'options', u'value': u'{d = 0x622320}'}, {u'type': u'KateApp * const', u'name': u'this', u'value': '0x7fffffffdc10', u'arg': '1'}, {u'type': u'KCmdLineArgs *', u'name': u'args', u'value': '0x6237f0', u'arg': '1'}])]
        #
        u = results[0][1]
        print("#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], u["from"]))

    def symbolFiles(self):
        results = self._gdb.miCommandOne("-file-list-symbol-files")
        #
        # [(u'variables', [{u'name': u'options', u'value': u'{d = 0x622320}'}, {u'type': u'KateApp * const', u'name': u'this', u'value': '0x7fffffffdc10', u'arg': '1'}, {u'type': u'KCmdLineArgs *', u'name': u'args', u'value': '0x6237f0', u'arg': '1'}])]
        #
        u = results[0][1]
        print("#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], u["from"]))

    def setExecAndSymbols(self, filename):
        results = self._gdb.miCommandOne("-file-exec-and-symbols {}".format(filename))
        #
        # Expected results = []
        #

    def setExecOnly(self, filename):
        results = self._gdb.miCommandOne("-file-exec-file {}".format(filename))
        #
        # Expected results = []
        #

    def setSymbolsOnly(self, filename):
        results = self._gdb.miCommandOne("-file-symbol-file {}".format(filename))
        #
        # Expected results = []
        #

class Python():
    """Model of embedded Python."""

    _gdb = None
    _connectionId = None

    def _pythonCommand(self, command):
        self._gdb._gdbThread.write("python {}\n".format(command))
        print("python "+command)
        self._gdb._gdbThread.waitForBytesWritten()

    def __init__(self, gdb):
        """Constructor."""
        self._gdb = gdb

    def enter(self, args):
        if not self._connectionId:
            command = ""
            command += "sys.path.insert(0, os.path.dirname('" + __file__ + "')); "
            command += "import gdb_execute; "
            command += "from " + self.__module__ + " import DebuggerKernel; "
            command += "app = DebuggerKernel(); app.start()"
            #
            # First time around...
            #
            self._pythonCommand(command)
            #
            # Wait for the line that announces the IPython connection string.
            #
            connectionInfo = "[IPKernelApp] --existing "
            error, lines = self._gdb.waitForPromptConsole(command, endLine = connectionInfo)
            if not lines[-1].startswith(connectionInfo):
                raise QGdbException("IPython connection error '{}'".format(lines))
            self._connectionId = lines[-1][len(connectionInfo):]
            self._gdb.dbg1("IPython connection='{}'".format(self._connectionId))
        return self._connectionId

    def exit(self):
        command = "%Quit"
        self._pythonCommand(command)
        error, lines = self._gdb.waitForPromptConsole("xxxxxxxxxxxxxxx")
        print("exiting!!!!!!!!!!!!!",error,lines)
        error, lines = self._gdb.waitForPromptConsole("yyyyyyyyyyy")
        print("exiting222!!!!!!!!!!!!!",error,lines)


class Stack():
    """Model of GDB stack."""

    _gdb = None

    def __init__(self, gdb):
        """Constructor."""
        self._gdb = gdb

    def depth(self, tid, maxFrames = None):
        if maxFrame != None:
            results = self._gdb.miCommandOne("-stack-info-depth --thread {} {}".format(tid, maxFrames))
        else:
            results = self._gdb.miCommandOne("-stack-info-depth --thread {}".format(tid))
        return result

    def frameInfo(self, tid, frame = None):
        """
        {u'from': u'/usr/lib/x86_64-linux-gnu/libQtGui.so.4', u'addr': '0x7ffff6d601b0', u'func': u'QWidget::QWidget(QWidgetPrivate&, QWidget*, QFlags<Qt::WindowType>)', u'level': '0'}
        """
        if frame != None:
            results = self._gdb.miCommandOne("-stack-info-frame --thread {} --frame {}".format(tid, frame))
        else:
            results = self._gdb.miCommandOne("-stack-info-frame --thread {}".format(tid))
        return results[u'frame']

    def frameVariables(self, tid, printValues, frame):
        """
        [{u'name': u'kateVersion', u'value': u'\\"3.9.2\\" = {[0] = 51 \'3\', [1] = 46 \'.\', [2] = 57 \'9\', [3] = 46 \'.\', [4] = 50 \'2\'}'}, {u'name': u'serviceName', u'value': u'\\"\\"'}, {u'name': u'start_session_set', u'value': u'<optimised out>'}, {u'name': u'start_session', u'value': u'\\"\\"'}, {u'name': u'app', u'value': u'{<KApplication> = {<No data fields>}, static staticMetaObject = {d = {superdata = 0x7ffff63e81e0 <KApplication::staticMetaObject>, stringdata = 0x7ffff6469d00 <qt_meta_stringdata_KateApp> \\"KateApp\\", data = 0x7ffff6469d60 <qt_meta_data_KateApp>, extradata = 0x7ffff667cce0 <KateApp::staticMetaObjectExtraData>}}, static staticMetaObjectExtraData = {objects = 0x0, static_metacall = 0x7ffff6433810 <KateApp::qt_static_metacall(QObject*, QMetaObject::Call, int, void**)>}, m_shouldExit = false, m_args = 0x7ffff7bcfd28, m_application = 0x1, m_docManager = 0x20a240, m_pluginManager = 0x0, m_sessionManager = 0x0, m_adaptor = 0x400688 <_start>, m_mainWindows = QList<KateMainWindow *> = {[0] = 0x0, ...}, m_mainWindowsInterfaces = QList<Kate::MainWindow *> = {[0] = <error reading variable>'}, ..., {u'name': u'argv', u'value': '0x7fffffffdd00', u'arg': '1'}]
        """
        results = self._gdb.miCommandOne("-stack-list-variables --thread {} --frame {} {}".format(tid, frame, printValues))
        return results[u'variables']

    def stackArguments(self, tid, printValues, lowFrame = None, highFrame = None):
        if lowFrame != None and highFrame != None:
            results = self._gdb.miCommandOne("-stack-list-arguments --thread {} {} {} {}".format(tid, printValues, lowFrame, highFrame))
        elif lowFrame == None and highFrame == None:
            results = self._gdb.miCommandOne("-stack-list-arguments --thread {} {}".format(tid, printValues))
        else:
            raise QGdbException("Invalid frame numbers {}, {}".format(lowFrame, highFrame))
        #
        # Suppress null output.
        #
        # [('stack-args', [('frame', {'args': [], 'level': '0'}), ('frame', {'args': [], 'level': '1'}), ('frame', {'args': [], 'level': '2'}), ('frame', {'args': [], 'level': '3'}), ('frame', {'args': [], 'level': '4'}), ('frame', {'args': [], 'level': '5'}), ('frame', {'args': [], 'level': '6'}), ('frame', {'args': [{'name': 'this', 'value': '0x7fffffffdc10'}, {'name': 'args', 'value': '0x6237f0'}], 'level': '7'}), ('frame', {'args': [{'name': 'argc', 'value': '<optimised out>'}, {'name': 'argv', 'value': '0x7fffffffdd00'}], 'level': '8'}), ('frame', {'args': [], 'level': '9'}), ('frame', {'args': [], 'level': '10'})])]
        #
        results = results[0][1]
        #
        # Print rows.
        #
        for f, v in results:
            level = v["level"]
            args = v["args"]
            args = ", ".join(["{}={}".format(d["name"], d["value"]) for d in args])
            print("#{} {}".format(level, args))

    def stackFrames(self, tid, lowFrame = None, highFrame = None):
        """
        [{u'frame': {u'from': u'/usr/lib/x86_64-linux-gnu/libQtGui.so.4', u'addr': '0x7ffff6d601b0', u'func': u'QWidget::QWidget(QWidgetPrivate&, QWidget*, QFlags<Qt::WindowType>)', u'level': '0'}}, ..., {u'frame': {u'addr': '0x4006b1', u'func': u'_start', u'level': '10'}}]
        """
        if lowFrame != None and highFrame != None:
            results = self._gdb.miCommandOne("-stack-list-frames --thread {} {} {}".format(tid, lowFrame, highFrame))
        elif lowFrame == None and highFrame == None:
            results = self._gdb.miCommandOne("-stack-list-frames --thread {}".format(tid))
        else:
            raise QGdbException("Invalid frame numbers {}, {}".format(lowFrame, highFrame))
        return results[u'stack']

class Threads():
    """Model of GDB threads."""

    _gdb = None

    def __init__(self, gdb):
        """Constructor."""
        self._gdb = gdb

    def list(self, id):
        """
        (current-thread-id, [{u'core': '0', u'target-id': u'Thread 0x7ffff7fe1780 (LWP 20450)', u'name': u'kate', u'frame': {u'args': [], u'from': u'/usr/lib/x86_64-linux-gnu/libQtGui.so.4', u'addr': '0x7ffff6d601b0', u'func': u'QWidget::QWidget(QWidgetPrivate&, QWidget*, QFlags<Qt::WindowType>)', u'level': '0'}, u'state': u'stopped', u'id': '1'}])
        """
        if id:
            results = self._gdb.miCommandOne("-thread-info {}".format(id))
            currentThread = None
        else:
            results = self._gdb.miCommandOne("-thread-info")
            currentThread = results[u'current-thread-id']
        return (currentThread, results[u'threads'])

class QGdbInterpreter(DebuggerIo):
    """Qt wrapper for GDB.

    The key features are:

        1. The Pythonic wrapper.

        2. Program execution is via the GDB/MI interpreter which allows
        unambiguous separation of output emitted by the running inferior from
        GDB's own output.

        3. Every other operation that can be implemented via the GDB/MI
        interpreter is also handled that way.

        4. All other access is via the GDB console interpreter, but commands
        are wrapped as needed to provide complete integration with Python
        language features behind a Python CLI:

            - All file commands support environment variables.
    """

    _breakpoints = None
    _data = None
    _programControl = None
    _python = None
    _stack = None
    _threads = None

    def __init__(self, arguments, verbose = 0):
        """Constructor.

        @param _gdbThreadStarted    Signal completion via semaphore.
        @param arguments        GDB start command.
        """
        _gdbThreadStarted = QSemaphore()
        super(QGdbInterpreter, self).__init__(_gdbThreadStarted, arguments)
        self.start()
        _gdbThreadStarted.acquire()
        #
        # Subprocess is running.
        #
        self._breakpoints = Breakpoints(self)
        self._data = Data(self)
        self._programControl = ProgramControl(self)
        self._python = Python(self)
        self._stack = Stack(self)
        self._threads = Threads(self)
