#
# Copyright 2012, 2013, Shaheed Haque <srhaque@theiet.org>.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# The thread-based I/O here was inspired by, and is in places
# derived from, pygdb (http://code.google.com/p/pygdb/).

from __future__ import print_function
import logging
import os

from PyQt4.QtCore import *
from PyKDE4.kdecore import *
import sys
sys.argv = ["gdb"]
from IPython.zmq.ipkernel import IPKernelApp

from miparser import MiParser


def dbg0(msg, *args):
    print("ERR-0", msg.format(*args))

def dbg1(msg, *args):
    print("DBG-1", msg.format(*args))

def dbg2(msg, *args):
    pass #print("DBG-2", msg.format(*args))

class QGdbException(Exception):
    pass

class QGdbInterrupted(QGdbException):
    pass

class QGdbTimeoutError(QGdbException):
    pass

class QGdbExecuteError(QGdbException):
    pass

class DebuggerPythonKernel():
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
        dbg0('shell being stopped is', self._app.shell)
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
    """Thread handling IO with the running inferior."""
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
        try:
            while not (self.parent()._interruptPending):
                line = self._masterFd.readline()
                if not line:
                    break
                self.parent().gdbStreamInferior.emit(line[:-1])
        except Exception as e:
            dbg0("unexpected exception: {} {}", self, e)
        else:
            dbg0("thread exit: {}", self)

    def interruptWait(self):
        """Interrupt an in-progress wait for response from GDB."""
        self._interruptPending = True

    def ttyName(self):
        return self._pty.ttyName()

class DebuggerIo(QObject):
    """A procedural interface to GDB running as a subprocess.
    A second thread is used to handle I/O with a direct inferior if needed.
    """
    _gdbThread = None
    _inferiorThread = None
    _interruptPending = None
    arguments = None
    _miToken = None
    def __init__(self, arguments, earlyConsolePrint, verbose = 0):
        """Constructor.

        @param arguments        GDB start command.
        """
        super(DebuggerIo, self).__init__()
        self.miParser = MiParser()
        self.arguments = arguments
        self.arguments.append("--interpreter=mi")
        self._miToken = 0
        self.onUnknownEvent.connect(self.unknownEvent)
        self._gdbThread = QProcess()
        self._gdbThread.setProcessChannelMode(QProcess.MergedChannels)
        self._gdbThread.error.connect(self.gdbProcessError)
        self._gdbThread.finished.connect(self.gdbProcessFinished)
        self._gdbThread.readyReadStandardOutput.connect(self.gdbProcessReadyReadStandardOutput)
        self._gdbThread.started.connect(self.gdbProcessStarted)
        self._gdbThread.stateChanged.connect(self.gdbProcessStateChanged)
        #
        # Output from the GDB process is read in realtime and written to the
        # self._lines list. This list is protected by the _linesMutex from the
        # user-called reader in waitForPrompt().
        #
        self._lines = []
        self._linesMutex = QMutex()
        #
        # Start.
        #
        self._gdbThread.start(self.arguments[0], self.arguments[1:])
        self._gdbThread.waitForStarted()
        self.waitForPrompt("", None, earlyConsolePrint)

    def interruptWait(self):
        """Interrupt an in-progress wait for response from GDB."""
        self._interruptPending = True

    def unknownEvent(self, key, args):
        dbg1("unknown event: {}, {}", key, args)

    def startIoThread(self):
        self._inferiorThread = InferiorIo(self)
        return self._inferiorThread.ttyName()

    def consoleCommand(self, command, captureConsole = False):
        """Execute a non-MI command using the GDB/MI interpreter."""
        dbg1("consoleCommand: {}", command)
        if captureConsole:
            self._captured = []
            self.waitForResults("", command, self.consoleCapture)
            return self._captured
        else:
            return self.waitForResults("", command, None)

    def consoleCapture(self, line):
        self._captured.append(line)

    def miCommand(self, command):
        """Execute a MI command using the GDB/MI interpreter."""
        self._miToken += 1
        command = "{}{}".format(self._miToken, command)
        dbg1("miCommand: '{}'", command)
        return self.waitForResults(str(self._miToken), command, None)

    def miCommandOne(self, command):
        """A specialisation of miCommand() where we expect exactly one result record."""
        records = self.miCommand(command)
        return records

    def miCommandExec(self, command, args):
        self.miCommandOne(command)

    def waitForResults(self, token, command, captureConsole, endLine = None, timeoutMs = 10000):
        """Wait for and check results from GDB.

        @return The result dictionary, or any captureConsole'd output.
        """
        self._gdbThread.write(command + "\n")
        self._gdbThread.waitForBytesWritten()
        records = self.waitForPrompt(token, command, captureConsole, endLine, timeoutMs)
        status, result = records[-1]
        del records[-1]
        if status:
            raise QGdbException("{}: unexpected status {}, {}, {}".format(command, status, result, records))
        #
        # Return the result information and any preceding records.
        #
        if captureConsole:
            if result:
                raise QGdbException("{}: unexpected result {}, {}".format(command, result, records))
            return records
        else:
            if records:
                raise QGdbException("{}: unexpected records {}, {}".format(command, result, records))
            return result

    def waitForPrompt(self, token, why, captureConsole, endLine = None, timeoutMs = 10000):
        """Read responses from GDB until a prompt, or interrupt.

        @return lines   Each entry in the lines array is either a console string or a
                        parsed dictionary of output. The last entry should be a result.
        """
        prompt = "(gdb) "
        foundResultOfCommand = not why
        result = []
        lines = []
        maxTimeouts = timeoutMs / 100
        dbg1("reading for: {}", why)
        self._interruptPending = False
        while True:
            self._linesMutex.lock()
            tmp = lines
            lines = self._lines
            self._lines = tmp
            self._linesMutex.unlock()
            while not lines and not self._interruptPending and maxTimeouts:
                self._gdbThread.waitForReadyRead(100)
                maxTimeouts -= 1
                self._linesMutex.lock()
                tmp = lines
                lines = self._lines
                self._lines = tmp
                self._linesMutex.unlock()
            if lines:
                for i in range(len(lines)):
                    #
                    # TODO: check what IPython does now.
                    #
                    line = lines[i]
                    if endLine and line.startswith(endLine):
                        #
                        # Yay, got to the end!
                        #
                        dbg1("TODO: check what IPython does: {}: all lines read: {}", why, len(lines))
                        #
                        # Save any unread lines for next time.
                        #
                        tmp = lines[i + 1:]
                        dbg0("push back {} lines: '{}'", len(tmp), tmp)
                        self._linesMutex.lock()
                        tmp.extend(self._lines)
                        self._lines = tmp
                        self._linesMutex.unlock()
                        result.append(line)
                        return result
                    elif line == prompt:
                        if foundResultOfCommand:
                            #
                            # Yay, got a prompt *after* the result record => got to the end!
                            #
                            dbg1("{}: all lines read: {}", why, len(lines))
                            #
                            # Save any unread lines for next time, but discard this one
                            #
                            tmp = lines[i + 1:]
                            if tmp:
                                dbg0("push back {} lines: '{}'", len(tmp), tmp)
                            self._linesMutex.lock()
                            tmp.extend(self._lines)
                            self._lines = tmp
                            self._linesMutex.unlock()
                            return result
                        else:
                            dbg1("ignored prompt")
                    elif line[0] == "~":
                        line = self.parseStringRecord(line[1:])
                        #
                        # GDB console stream record.
                        #
                        if captureConsole:
                            captureConsole(line)
                        else:
                            self.gdbStreamConsole.emit(line)
                    elif line.startswith(token + "^"):
                        #
                        # GDB result-of-command record.
                        #
                        line = line[len(token) + 1:]
                        result.append(self.parseResultRecord(line))
                        foundResultOfCommand = True
                    else:
                        result.append(line)
                        dbg0("{}: unexpected record string '{}'", why, line)
                    #
                    # We managed to read a line, so reset the timeout.
                    #
                    maxTimeouts = timeoutMs / 100
                lines = []
            elif self._interruptPending:
                #
                # User got fed up. Note, there may be more to read!
                #
                raise QGdbInterrupted("{}: interrupt after {} lines read, {}".format(why, len(result), result))
            elif not maxTimeouts:
                #
                # Caller got fed up. Note, there may be more to read!
                #
                raise QGdbTimeoutError("{}: timeout after {} lines read, {}".format(why, len(result), result))

    def parseStringRecord(self, line):
        return self.miParser.parse("t=" + line)['t'].strip()

    def parseOobRecord(self, line):
        """GDB/MI OOB record."""
        dbg2("OOB string {}", line)
        tuple = line.split(",", 1)
        if len(tuple) > 1:
            tuple[1] = self.miParser.parse(tuple[1])
        else:
            tuple.append({})
        dbg1("OOB record '{}'", tuple[0])
        return tuple

    def parseResultRecord(self, line):
        """GDB/MI Result record.

        @param result   "error" for ^error
                        "exit" for ^exit
                        "" for normal cases (^done, ^running, ^connected)
        @param data     "c-string" for ^error
                        "results" for ^done
        """
        dbg2("Result string {}", line)
        tuple = line.split(",", 1)
        if tuple[0] in ["done", "running"]:
            tuple[0] = ""
        elif tuple[0] == "error":
            raise QGdbExecuteError(self.miParser.parse(tuple[1])["msg"])
        else:
            raise QGdbException("Unexpected result string '{}'".format(line))
        if len(tuple) > 1:
            tuple[1] = self.miParser.parse(tuple[1])
        else:
            tuple.append({})
        dbg1("Result record '{}', '{}'", tuple[0], tuple[1].keys())
        return tuple

    def signalEvent(self, event, args):
        """Signal any interesting events."""
        try:
            if event == "stopped":
                self.onStopped.emit(args)
            elif event == "running":
                #
                # This is a string thread id, to allow for the magic value "all".
                # TODO: A more Pythonic model.
                #
                tid = args["thread-id"]
                self.onRunning.emit(tid)
            elif event.startswith("thread-group"):
                tgid = args["id"]
                if event == "thread-group-added":
                    self.onThreadGroupAdded.emit(tgid)
                elif event == "thread-group-removed":
                    self.onThreadGroupRemoved.emit(tgid)
                elif event == "thread-group-started":
                    self.onThreadGroupStarted.emit(tgid, int(args["pid"]))
                elif event == "thread-group-exited":
                    try:
                        exitCode = int(args["exit-code"])
                    except KeyError:
                        exitCode = 0
                    self.onThreadGroupExited.emit(tgid, exitCode)
                else:
                    self.onUnknownEvent.emit(event, args)
            elif event.startswith("thread"):
                tid = int(args["id"])
                if event == "thread-created":
                    tgid = args["group-id"]
                    self.onThreadCreated.emit(tid, tgid)
                elif event == "thread-exited":
                    tgid = args["group-id"]
                    self.onThreadExited.emit(tid, tgid)
                elif event == "thread-selected":
                    self.onThreadSelected.emit(tid)
                else:
                    self.onUnknownEvent.emit(event, args)
            elif event.startswith("library"):
                lid = args["id"]
                hostName = args["host-name"]
                targetName = args["target-name"]
                tgid = args["thread-group"]
                if event == "library-loaded":
                    self.onLibraryLoaded.emit(lid, hostName, targetName, int(args["symbols-loaded"]), tgid)
                elif event == "library-unloaded":
                    self.onLibraryUnloaded.emit(lid, hostName, targetName, tgid)
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
        except Exception as e:
            dbg0("unexpected exception: {}: {}", self, e)

    @pyqtSlot(QProcess.ProcessError)
    def gdbProcessError(self, error):
        dbg0("gdbProcessError: {}", error)

    @pyqtSlot(int, QProcess.ExitStatus)
    def gdbProcessFinished(self, exitCode, exitStatus):
        dbg2("gdbProcessFinished: {}, {}", exitCode, exitStatus)

    @pyqtSlot()
    def gdbProcessReadyReadStandardOutput(self):
        dbg2("gdbProcessReadyReadStandardOutput")
        while self._gdbThread.canReadLine():
            line = self._gdbThread.readLine()
            line = str(line[:-1], "utf-8")
            #
            # What kind of line is this, one we have to save, or one we have
            # to emit right away?
            #
            if line[0] == "@":
                line = self.parseStringRecord(line[1:])
                #
                # Target stream record. Emit now!
                #
                self.gdbStreamInferior.emit(line)
            elif line[0] == "&":
                line = self.parseStringRecord(line[1:])
                #
                # GDB log stream record. Emit now!
                #
                self.gdbStreamLog.emit(line)
            elif line[0] in ["*", "="]:
                #
                # GDB OOB stream record. TODO: does "*" mean inferior state change to stopped?
                #
                line = line[1:]
                tuple = self.parseOobRecord(line)
                self.signalEvent(tuple[0], tuple[1])
            else:
                #
                # GDB console stream record (~),
                # GDB result-of-command record (token^),
                # or something else (e.g. prompt).
                #
                self._linesMutex.lock()
                self._lines.append(line)
                self._linesMutex.unlock()

    @pyqtSlot()
    def gdbProcessStarted(self):
        dbg2("gdbProcessStarted")

    @pyqtSlot(QProcess.ExitStatus)
    def gdbProcessStateChanged(self, newState):
        dbg2("gdbProcessStateChanged: {}", newState)

    """GDB/MI Stream record, GDB console output."""
    gdbStreamConsole = pyqtSignal('QString')

    """GDB/MI Stream record, GDB target output."""
    gdbStreamInferior = pyqtSignal('QString')

    """GDB/MI Stream record, GDB log output."""
    gdbStreamLog = pyqtSignal('QString')

    onUnknownEvent = pyqtSignal('QString', dict)

    """running,thread-id="all". """
    onRunning = pyqtSignal('QString')
    """stopped,reason="breakpoint-hit",disp="del",bkptno="1",frame={addr="0x4006b0",func="main",args=[{name="argc",value="1"},{name="argv",value="0x7fd48"}],file="dummy.cpp",fullname="dummy.cpp",line="3"},thread-id="1",stopped-threads="all",core="5". """
    onStopped = pyqtSignal(dict)

    """thread-group-added,id="id". """
    onThreadGroupAdded = pyqtSignal('QString')
    """thread-group-removed,id="id". """
    onThreadGroupRemoved = pyqtSignal('QString')
    """thread-group-started,id="id",pid="pid". """
    onThreadGroupStarted = pyqtSignal('QString', int)
    """thread-group-exited,id="id"[,exit-code="code"]. """
    onThreadGroupExited = pyqtSignal('QString', int)

    """thread-created,id="id",group-id="gid". """
    onThreadCreated = pyqtSignal(int, 'QString')
    """thread-exited,id="id",group-id="gid". """
    onThreadExited = pyqtSignal(int, 'QString')
    """thread-selected,id="id". """
    onThreadSelected = pyqtSignal(int)

    """library-loaded,id="id",target-name,host-name,symbols-loaded[,thread-group].
    Note: symbols-loaded is not used"""
    onLibraryLoaded = pyqtSignal('QString', 'QString', 'QString', 'bool', 'QString')
    """library-unloaded,id="id",target-name,host-name[,thread-group]. """
    onLibraryUnloaded = pyqtSignal('QString', 'QString', 'QString', 'QString')

    """breakpoint-created,bkpt={...}. """
    onBreakpointCreated = pyqtSignal(dict)
    """breakpoint-modified,bkpt={...}. """
    onBreakpointModified = pyqtSignal(dict)
    """breakpoint-deleted,bkpt={...}. """
    onBreakpointDeleted = pyqtSignal(dict)

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
        self._out("#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], u["from"]))

    def symbolFiles(self):
        results = self._gdb.miCommandOne("-file-list-symbol-files")
        #
        # [(u'variables', [{u'name': u'options', u'value': u'{d = 0x622320}'}, {u'type': u'KateApp * const', u'name': u'this', u'value': '0x7fffffffdc10', u'arg': '1'}, {u'type': u'KCmdLineArgs *', u'name': u'args', u'value': '0x6237f0', u'arg': '1'}])]
        #
        u = results[0][1]
        self._out("#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], u["from"]))

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
        dbg0("python "+command)
        self._gdb._gdbThread.waitForBytesWritten()

    def __init__(self, gdb):
        """Constructor."""
        self._gdb = gdb

    def enter(self, args):
        if not self._connectionId:
            command = ""
            command += "sys.path.insert(0, os.path.dirname('" + __file__ + "')); "
            command += "import gdb_execute; "
            command += "from " + self.__module__ + " import DebuggerPythonKernel; "
            command += "app = DebuggerPythonKernel(); app.start()"
            #
            # First time around...
            #
            self._pythonCommand(command)
            #
            # Wait for the line that announces the IPython connection string.
            #
            connectionInfo = "[IPKernelApp] --existing "
            error, lines = self._gdb.waitForPrompt(None, command, endLine = connectionInfo)
            if not lines[-1].startswith(connectionInfo):
                raise QGdbException("IPython connection error '{}'".format(lines))
            self._connectionId = lines[-1][len(connectionInfo):]
            self._gdb.dbg1("IPython connection='{}'".format(self._connectionId))
        return self._connectionId

    def exit(self):
        command = "%Quit"
        self._pythonCommand(command)
        error, lines = self._gdb.waitForPrompt(None, "xxxxxxxxxxxxxxx")
        dbg0("exiting!!!!!!!!!!!!!",error,lines)
        error, lines = self._gdb.waitForPrompt(None, "yyyyyyyyyyy")
        dbg0("exiting222!!!!!!!!!!!!!",error,lines)

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
            self._out("#{} {}".format(level, args))

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

    def __init__(self, arguments, earlyConsolePrint, verbose = 0):
        """Constructor.

        @param arguments        GDB start command.
        """
        super(QGdbInterpreter, self).__init__(arguments, earlyConsolePrint)
        #
        # Subprocess is running.
        #
        self._breakpoints = Breakpoints(self)
        self._data = Data(self)
        self._programControl = ProgramControl(self)
        self._python = Python(self)
        self._stack = Stack(self)
        self._threads = Threads(self)
