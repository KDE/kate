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

from PyQt4.QtCore import *
from PyKDE4.kdecore import *
import curses.ascii
import os

from miparser import MiParser

class QGdbException(Exception):
	pass

class QGdbTimeoutError(QGdbException):
	pass

class QGdbInvalidResults(QGdbException):
	pass

class QGdbExecuteError(QGdbException):
	pass

class InferiorIo(QThread):
	_pty = None
	def __init__(self, parent):
		super(InferiorIo, self).__init__(parent)
		# TODO: convert to KPtyDevice
		self._pty = KPty()
		if not self._pty.open():
			raise QGdbException("Cannot open pty")
		#if not self._pty.openSlave():
		#	raise QGdbException("Cannot open pty slave")
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
		print "inferior reader done!!!!"

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

		@param _gdbThreadStarted	Signal completion via semaphore.
		@param arguments		GDB start command.
		"""
		super(DebuggerIo, self).__init__()
		self.miParser = MiParser()
		self._gdbThreadStarted = gdbThreadStarted
		self.arguments = arguments
		self._miToken = 0

	def run(self):
		try:
			#self._gdbThread = pygdb.Gdb(GDB_CMDLINE, handler = self, verbose = verbose)
			print "QGDB command line", self.arguments
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
			print "TODO make signal work", str(e)
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

	def waitForPromptConsole(self, why, timeoutMs = 10000):
		"""Read responses from GDB until a prompt, or interrupt.

		@return (error, lines)	Where error is None (normal prompt seen),
					curses.ascii.ESC (user interrupt) or
					curses.ascii.CAN (caller timeout)
		"""
		prompt = "(gdb) "
		lines = []
		maxTimeouts = timeoutMs / 100
		self.dbg1("reading for: {}", why)
		self._interruptPending = False
		while True:
			while not self._gdbThread.canReadLine() and \
					self._gdbThread.peek(len(prompt)) != prompt and \
					not self._interruptPending and \
					maxTimeouts:
				self._gdbThread.waitForReadyRead(100)
				maxTimeouts -= 1
			if self._gdbThread.canReadLine():
				line = self._gdbThread.readLine()
				lines.append(unicode(line[:-1], "utf-8"))
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

	def waitForPromptMi(self, token, why, timeoutMs = 10000):
		"""Read responses from GDB until a prompt, or interrupt.

		@return (error, lines)	Where error is None (normal prompt seen),
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

		@param result	"error" for ^error
				"exit" for ^exit
				"" for normal cases (^done, ^running, ^connected)
		@param data	"c-string" for ^error
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
		print "ERR-0", msg.format(*args)

	def dbg1(self, msg, *args):
		print "DBG-1", msg.format(*args)

	def dbg2(self, msg, *args):
		print "DBG-2", msg.format(*args)

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
		print results["bkpt"]
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
		print results[0][1]
		return results[0][1]

	def breakpointDelete(self, id):
		results = self._gdb.miCommandOne("-break-delete {}".format(id))
		print results

	def tracepointDelete(self, id):
		self.breakpointDelete(id)

	def setAfter(self, id, count):
		# TODO
		#if id is watchpoint:
		#	results = self._gdb.miCommandOne("-break-passcount {} {}".format(id, count))
		#else:
		results = self._gdb.miCommandOne("-break-after {} {}".format(id, count))
		print results

	def setCommands(self, id, commands):
		commands = " ".join(["\"{}\"".format(c) for c in commands])
		results = self._gdb.miCommandOne("-break-commands {} {}".format(id, commands))
		print results

	def setCondition(self, id, condition):
		results = self._gdb.miCommandOne("-break-condition {} {}".format(id, condition))
		print results

	def setEnable(self, id, enabled):
		if enabled:
			results = self._gdb.miCommandOne("-break-enable {}".format(id))
		else:
			results = self._gdb.miCommandOne("-break-disable {}".format(id))
		print results

	def list(self, id):
		if id:
			results = self._gdb.miCommandOne("-break-info {}".format(id))
		else:
			results = self._gdb.miCommandOne("-break-info")
		#
		# {u'BreakpointTable': {u'body': [{u'bkpt': {u'disp': u'keep', u'addr': u'<MULTIPLE>', u'type': u'breakpoint', u'enabled': u'y', u'number': '2', u'original-location': u'QWidget::QWidget', u'times': '0'}}, {u'at': u'<QWidget::QWidget(QWidgetPrivate&, QWidget*, QFlags<Qt::WindowType>)>', u'enabled': u'y', u'number': u'2.1', u'addr': '0x7ffff6d601b0'}, {u'at': u'<QWidget::QWidget(QWidget*, char const*, QFlags<Qt::WindowType>)>', u'enabled': u'y', u'number': u'2.2', u'addr': '0x7ffff6d60270'}, {u'at': u'<QWidget::QWidget(QWidget*, QFlags<Qt::WindowType>)>', u'enabled': u'y', u'number': u'2.3', u'addr': '0x7ffff6d603c0'}], u'hdr': [{u'width': '7', u'colhdr': u'Num', u'col_name': u'number', u'alignment': '-1'}, {u'width': '14', u'colhdr': u'Type', u'col_name': u'type', u'alignment': '-1'}, {u'width': '4', u'colhdr': u'Disp', u'col_name': u'disp', u'alignment': '-1'}, {u'width': '3', u'colhdr': u'Enb', u'col_name': u'enabled', u'alignment': '-1'}, {u'width': '18', u'colhdr': u'Address', u'col_name': u'addr', u'alignment': '-1'}, {u'width': '40', u'colhdr': u'What', u'col_name': u'what', u'alignment': '2'}], u'nr_cols': '6', u'nr_rows': '1'}}
		#
		results = results[u'BreakpointTable'][u'body']
		if id:
			results = [row for row in results if row[1]["number"] == id]
		if not len(results):
			return
		#
		# Print rows.
		#
		fmt = "{:<7} {:<14} {:<4} {:<3} {}"
		print fmt.format("Num", "Type", "Disp", "Enb", "Where")
		for u in results:
			try:
				u = u[u'bkpt']
				try:
					location = u["fullname"]
				except KeyError:
					try:
						location = u["file"]
					except KeyError:
						try:
							location = u["original-location"]
						except KeyError:
							location = u["at"]
							u["type"] = "";
							u["disp"] = "";
				try:
					addr = u["addr"]
				except KeyError:
					addr = 0
				try:
					func = u["func"]
					line = u["line"]
				except KeyError:
					func = ""
					line = 0
				location = "{} {} at {}:{}".format(addr, func, location, line)
				print fmt.format(u["number"], u["type"], u["disp"], u["enabled"], location)
				try:
					times = u["times"]
					if times != "0":
						print "        breakpoint already hit {} times".format(times)
				except KeyError:
					pass
			except KeyError:
				#
				# Not a standalone breakpoint, just an overload of one.
				#
				location = "{} {}".format(u["addr"], u["at"])
				print fmt.format(u["number"], "", "", u["enabled"], location)

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
		self._getRegisterNames()
		if regName:
			try:
				regNo = self._registerNames[regName]
			except:
				raise QGdbException("Invalid register name {}".format(regName))
			results = self._gdb.miCommandOne("-data-list-register-values {} {}".format(fmt, regNo))
		else:
			results = self._gdb.miCommandOne("-data-list-register-values {}".format(fmt))
		#
		# {u'register-values': [{u'number': '0', u'value': '0x7ffff77b0d90'}, {u'number': '1', u'value': '0x7158b0'}, ..., {u'number': '140', u'value': '0x3feffff400000000'}, {u'number': '141', u'value': '0x3ff6980000000000'}]}
		#
		results = results[u'register-values']
		#
		# Print rows.
		#
		for u in results:
			print self._registerNumbers[int(u[u'number'])], u[u'value']

	def disassemble(self, mode, start_addr = _nextStart, end_addr = _nextEnd, filename = _filename, linenum = _nextLine, lines = _lines):
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
		#
		# {u'asm_insns': [{u'inst': u'push   %rax', u'address': '0x7fffffffdc10'}, {u'inst': u'cmpl   $0x7ffff6,(%rsi)', u'address': '0x7fffffffdc11'}]}
		#
		result = result[u'asm_insns']
		for u in result:
			print u[u'address'], u[u'inst']

	def evalute(self, expr):
		return self._gdb.miCommandOne('-data-evaluate-expression', expr)

	def data_list_changed_registers(self):
		return self._gdb.miCommandOne('-data-list-changed-registers')

	def readMemory(self, address = _nextStart, word_format = _wordFormat, word_size = _wordSize, nr_rows = _rows, nr_cols = _columns, offset_bytes = _offsetBytes, aschar = _asChar):
		if not address or not word_format or not word_size or not nr_rows or not nr_cols:
			raise QGdbException("Bad memory args")
		options = ""
		if offset_bytes:
			options += "-o " + offset_bytes
		results = self._gdb.miCommandOne("-data-read-memory {} {} {} {} {} {}".format(options, address, word_format, word_size, nr_rows, nr_cols))
		#
		# {u'prev-page': '0x7fffffffdbd0', u'prev-row': '0x7fffffffdc08', u'addr': '0x7fffffffdc10', u'nr-bytes': '64', u'total-bytes': '64', u'next-row': '0x7fffffffdc18', u'memory': [{u'data': ['0xf63e8150', '0x7fff'], u'addr': '0x7fffffffdc10'}, {u'data': ['0x645c10', '0x0'], u'addr': '0x7fffffffdc18'}, {u'data': ['0x69b220', '0x0'], u'addr': '0x7fffffffdc20'}, {u'data': ['0x0', '0x0'], u'addr': '0x7fffffffdc28'}, {u'data': ['0xf7bcfd28', '0x7fff'], u'addr': '0x7fffffffdc30'}, {u'data': ['0x1', '0x0'], u'addr': '0x7fffffffdc38'}, {u'data': ['0x20a240', '0x0'], u'addr': '0x7fffffffdc40'}, {u'data': ['0x0', '0x0'], u'addr': '0x7fffffffdc48'}], u'next-page': '0x7fffffffdc50'}
		#
		self._nextStart = results[u'next-page']
		self._wordFormat = word_format
		self._wordSize = word_size
		self._rows = nr_rows
		self._columns = nr_cols
		self._offsetBytes = offset_bytes
		self._asChar = aschar
		bytesRequested = results[u'total-bytes']
		bytesRead = results[u'nr-bytes']
		results = results[u'memory']
		for u in results:
			print u[u'addr'], u[u'data']

class ProgramControl():
	"""Model of GDB files."""

	_gdb = None

	def __init__(self, gdb):
		"""Constructor."""
		self._gdb = gdb

	def allSources(self):
		results = self._gdb.miCommandOne("-file-list-exec-source-files")
		#
		# [(u'files', [{u'fullname': u'/home/srhaque/kdebuild/kate/kate/app/kate_dummy.cpp', u'file': u'/home/srhaque/kdebuild/kate/kate/app/kate_dummy.cpp'}])]]
		#
		results = results[0][1]
		for u in results:
			try:
				file = u["fullname"]
			except KeyError:
				file = u["file"]
			print file

	def currentSource(self):
		results = self._gdb.miCommandOne("-file-list-exec-source-file")
		#
		# [(u'line', '1'), (u'file', u'/home/srhaque/kdebuild/kate/kate/app/kate_dummy.cpp'), (u'fullname', u'/home/srhaque/kdebuild/kate/kate/app/kate_dummy.cpp'), (u'macro-info', '0')]]
		#
		print "Current source file is {}".format(results[1][1])
		print "Located in {}".format(results[2][1])
		print "Contains {} lines.".format(results[0][1])
		if results[3][1] != "0":
			print "Does include preprocessor macro info."
		else:
			print "Does not include preprocessor macro info."

	def execSections(self):
		results = self._gdb.miCommandOne("-file-list-exec-sections")
		#
		# [(u'variables', [{u'name': u'options', u'value': u'{d = 0x622320}'}, {u'type': u'KateApp * const', u'name': u'this', u'value': '0x7fffffffdc10', u'arg': '1'}, {u'type': u'KCmdLineArgs *', u'name': u'args', u'value': '0x6237f0', u'arg': '1'}])]
		#
		u = results[0][1]
		print u
		print "#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], u["from"])

	def sharedLibraries(self):
		results = self._gdb.miCommandOne("-file-list-shared-libraries")
		#
		# [(u'variables', [{u'name': u'options', u'value': u'{d = 0x622320}'}, {u'type': u'KateApp * const', u'name': u'this', u'value': '0x7fffffffdc10', u'arg': '1'}, {u'type': u'KCmdLineArgs *', u'name': u'args', u'value': '0x6237f0', u'arg': '1'}])]
		#
		u = results[0][1]
		print u
		print "#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], u["from"])

	def symbolFiles(self):
		results = self._gdb.miCommandOne("-file-list-symbol-files")
		#
		# [(u'variables', [{u'name': u'options', u'value': u'{d = 0x622320}'}, {u'type': u'KateApp * const', u'name': u'this', u'value': '0x7fffffffdc10', u'arg': '1'}, {u'type': u'KCmdLineArgs *', u'name': u'args', u'value': '0x6237f0', u'arg': '1'}])]
		#
		u = results[0][1]
		print u
		print "#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], u["from"])

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
		print result
		return result

	def frameInfo(self, tid, frame = None):
		if frame != None:
			results = self._gdb.miCommandOne("-stack-info-frame --thread {} --frame {}".format(tid, frame))
		else:
			results = self._gdb.miCommandOne("-stack-info-frame --thread {}".format(tid))
		#
		# {u'frame': {u'from': u'/usr/lib/x86_64-linux-gnu/libQtGui.so.4', u'addr': '0x7ffff6d601b0', u'func': u'QWidget::QWidget(QWidgetPrivate&, QWidget*, QFlags<Qt::WindowType>)', u'level': '0'}}
		#
		u = results[u'frame']
		print "#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], u["from"])

	def frameVariables(self, tid, printValues, frame):
		results = self._gdb.miCommandOne("-stack-list-variables --thread {} --frame {} {}".format(tid, frame, printValues))
		#
		# {u'variables': [{u'name': u'kateVersion', u'value': u'\\"3.9.2\\" = {[0] = 51 \'3\', [1] = 46 \'.\', [2] = 57 \'9\', [3] = 46 \'.\', [4] = 50 \'2\'}'}, {u'name': u'serviceName', u'value': u'\\"\\"'}, {u'name': u'start_session_set', u'value': u'<optimised out>'}, {u'name': u'start_session', u'value': u'\\"\\"'}, {u'name': u'app', u'value': u'{<KApplication> = {<No data fields>}, static staticMetaObject = {d = {superdata = 0x7ffff63e81e0 <KApplication::staticMetaObject>, stringdata = 0x7ffff6469d00 <qt_meta_stringdata_KateApp> \\"KateApp\\", data = 0x7ffff6469d60 <qt_meta_data_KateApp>, extradata = 0x7ffff667cce0 <KateApp::staticMetaObjectExtraData>}}, static staticMetaObjectExtraData = {objects = 0x0, static_metacall = 0x7ffff6433810 <KateApp::qt_static_metacall(QObject*, QMetaObject::Call, int, void**)>}, m_shouldExit = false, m_args = 0x7ffff7bcfd28, m_application = 0x1, m_docManager = 0x20a240, m_pluginManager = 0x0, m_sessionManager = 0x0, m_adaptor = 0x400688 <_start>, m_mainWindows = QList<KateMainWindow *> = {[0] = 0x0, ...}, m_mainWindowsInterfaces = QList<Kate::MainWindow *> = {[0] = <error reading variable>'}, ..., {u'name': u'argv', u'value': '0x7fffffffdd00', u'arg': '1'}]}
		#
		results = [u for u in results[u'variables']]
		#
		# Print rows.
		#
		for u in results:
			try:
				print "arg {} {} = {} = {}".format(u["arg"], u["name"], u["type"], u["value"])
			except KeyError:
				try:
					print "{} = {} = {}".format(u["name"], u["type"], u["value"])
				except KeyError:
					print "{} = {}".format(u["name"], u["value"])

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
			print "#{} {}".format(level, args)

	def stackFrames(self, tid, lowFrame = None, highFrame = None):
		if lowFrame != None and highFrame != None:
			results = self._gdb.miCommandOne("-stack-list-frames --thread {} {} {}".format(tid, lowFrame, highFrame))
		elif lowFrame == None and highFrame == None:
			results = self._gdb.miCommandOne("-stack-list-frames --thread {}".format(tid))
		else:
			raise QGdbException("Invalid frame numbers {}, {}".format(lowFrame, highFrame))
		#
		# {u'stack': [{u'frame': {u'from': u'/usr/lib/x86_64-linux-gnu/libQtGui.so.4', u'addr': '0x7ffff6d601b0', u'func': u'QWidget::QWidget(QWidgetPrivate&, QWidget*, QFlags<Qt::WindowType>)', u'level': '0'}}, ..., {u'frame': {u'addr': '0x4006b1', u'func': u'_start', u'level': '10'}}]}
		#
		results = [u for u in results[u'stack']]
		#
		# Print rows.
		#
		for f in results:
			u = f[u'frame']
			try:
				location = u["from"]
			except KeyError:
				try:
					location = u["fullname"] + ":" + u["line"]
				except KeyError:
					try:
						location = u["file"] + ":" + u["line"]
					except KeyError:
						print "#{}  {} in {} ()".format(u["level"], u["addr"], u["func"])
						continue
			print "#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], location)

class Threads():
	"""Model of GDB threads."""

	_gdb = None

	def __init__(self, gdb):
		"""Constructor."""
		self._gdb = gdb

	def list(self, id):
		if id:
			results = self._gdb.miCommandOne("-thread-info {}".format(id))
			currentThread = None
		else:
			results = self._gdb.miCommandOne("-thread-info")
			currentThread = [u for u in results[u'current-thread-id']][0]
		#
		# {u'threads': [{u'core': '0', u'target-id': u'Thread 0x7ffff7fe1780 (LWP 20450)', u'name': u'kate', u'frame': {u'args': [], u'from': u'/usr/lib/x86_64-linux-gnu/libQtGui.so.4', u'addr': '0x7ffff6d601b0', u'func': u'QWidget::QWidget(QWidgetPrivate&, QWidget*, QFlags<Qt::WindowType>)', u'level': '0'}, u'state': u'stopped', u'id': '1'}], u'current-thread-id': '1'}]
		#
		results = [u for u in results[u'threads']]
		if id:
			results = [row for row in results if row["id"] == id]
		if not len(results):
			return
		#
		# Print rows.
		#
		fmt = "{:<1} {:<4} {:<37} {}"
		print fmt.format(" ", "Id", "Target Id", "Where")
		for v in results:
			if currentThread == v["id"]:
				active = "*"
			else:
				active = " "
			frame = v["frame"]
			args = frame["args"]
			args = ", ".join(["{}={}".format(d["name"], d["value"]) for d in args])
			try:
				location = frame["fullname"]
			except KeyError:
				try:
					location = frame["file"]
				except KeyError:
					location = frame["from"]
			try:
				line = frame["line"]
			except KeyError:
				line = ""
			location = "{}: {}({}) at {}:{}".format(frame["addr"], frame["func"], args, location, line)
			name = v["name"]
			if name:
				name += ", "
			else:
				name = ""
			print fmt.format(active, v["id"], name + v["target-id"], location)

class QGdbInterpreter(DebuggerIo):

	_breakpoints = None
	_data = None
	_programControl = None
	_stack = None
	_threads = None

	def __init__(self, arguments, verbose = 0):
		"""Constructor.

		@param _gdbThreadStarted	Signal completion via semaphore.
		@param arguments		GDB start command.
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
		self._stack = Stack(self)
		self._threads = Threads(self)
