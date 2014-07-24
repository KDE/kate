#
# Copyright 2009, 2013, Shaheed Haque <srhaque@theiet.org>.
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
#

from __future__ import print_function
import argparse
import atexit
import cmd
import re
import traceback

from IPython.frontend.terminal.console.interactiveshell import ZMQTerminalInteractiveShell
from IPython.lib.kernel import find_connection_file
from IPython.zmq.blockingkernelmanager import BlockingKernelManager
from PyQt4.QtCore import QCoreApplication, QObject

from gdb_command_db import GdbCommandDb
from qgdb import QGdbInterpreter

def dbg0(msg, *args):
    print("ERR-0", msg.format(*args))

def dbg1(msg, *args):
    print("DBG-1", msg.format(*args))

def dbg2(msg, *args):
    print("DBG-2", msg.format(*args))

class IPythonConsoleShell(ZMQTerminalInteractiveShell):
    """A simple console shell for IPython.

    References:

    - http://stackoverflow.com/questions/9977446/connecting-to-a-remote-ipython-instance
    - https://github.com/ipython/ipython/blob/master/IPython/zmq/blockingkernelmanager.py

    For the Qt version, see:

    - http://stackoverflow.com/questions/11513132/embedding-ipython-qt-console-in-a-pyqt-application
    """
    def __init__(self, *args, **kwargs):
        connection_file = find_connection_file(kwargs.pop("connection_file"))

        km = BlockingKernelManager(connection_file=connection_file)

        km.load_connection_file()
        heartbeat = True
        km.start_channels(hb=heartbeat)
        atexit.register(km.cleanup_connection_file)
        super(IPythonConsoleShell, self).__init__(kernel_manager = km)
        self.km = km

    def stop(self):
        print("IPythonConsoleShell stop()")
        self.exit_now = True
        #self.km.stop_channels()
        self.km.shutdown_kernel()
        self.ask_exit()

class MyArgs(argparse.ArgumentParser):
    def __init__(self, **kwargs):
        super(MyArgs, self).__init__(**kwargs)

    def format_usage(self):
        formatter = self._get_formatter()
        formatter._indent_increment = 4
        formatter.add_usage(self.usage, self._actions,
                self._mutually_exclusive_groups, "")
        return formatter.format_help()

    def format_help(self):
        formatter = self._get_formatter()
        formatter._indent_increment = 4

        # usage
        formatter.add_usage(self.usage, self._actions,
                self._mutually_exclusive_groups, "")

        # description
        formatter.add_text(self.description)

        # positionals, optionals and user-defined groups
        for action_group in self._action_groups:
            formatter.start_section(action_group.title)
            formatter.add_text(action_group.description)
            formatter.add_arguments(action_group._group_actions)
            formatter.end_section()

        # epilog
        formatter.add_text(self.epilog)

        # determine help from format above
        return formatter.format_help()

class Cli(cmd.Cmd):
    """Python CLI for GDB."""

    prompt = "(pygdb) "

    #
    # Our database of commands.
    #
    commandDb = None

    #
    # Commands which will have environment variable substitution applied.
    #
    filesCommands = None

    #
    # Output handling.
    #
    _out = None

    def __init__(self, arguments, printLine):
        cmd.Cmd.__init__(self)
        self._out = printLine
        self.gdb = QGdbInterpreter(arguments, printLine)
        self.createCommandDb()

    def createCommandDb(self):
        """Create a command database we can use to implement our CLI."""
        #
        # Ask GDB for all the commands it has.
        #
        helpText = self.gdb.consoleCommand("help all", True)
        self.commandDb = GdbCommandDb(helpText)
        self.findFilesCommand()
        #
        # Add in all our overrides; that's any routine starting doXXX.
        #
        customCommands = [c for c in dir(self) if c.startswith("do_")]
        for cmd in customCommands:
            self.commandDb.addCustom(getattr(self, cmd))
        #dbg0(self.commandDb)

    def findFilesCommand(self):
        """Make a list of each command which takes a file/path."""

        def matchClass(clazz_exact, arg, indentation, prefix, keyword, apropos, clazz, function):
            """
            Add contents of the database which are in the given clazz_exact to
            the files set.
            """
            if clazz == clazz_exact:
                arg[prefix + keyword] = apropos

        def matchRegExp(regexp, arg, indentation, prefix, keyword, apropos, clazz, function):
            """
            Add contents of the database which match the given regexp to the
            files set.
            """
            if regexp.search(keyword) or regexp.search(apropos):
                arg[prefix + keyword] = apropos

        #
        # Put all the commands we want to wrap into a dictinary, to avoid duplicates.
        #
        self.filesCommands = dict()
        self.commandDb.walk(matchClass, "files", self.filesCommands)
        self.commandDb.walk(matchRegExp, re.compile(" path", re.IGNORECASE), self.filesCommands)
        self.commandDb.walk(matchRegExp, re.compile(" file", re.IGNORECASE), self.filesCommands)

    #
    # See http://lists.baseurl.org/pipermail/yum-devel/2011-August/008495.html
    #
    def ____cmdloop(self):
        """ Sick hack for readline. """
        import __builtin__
        oraw_input = raw_input
        owriter    = sys.stdout
        _ostdout   = owriter  #.stream

        def _sick_hack_raw_input(prompt):
            sys.stdout = _ostdout
            #rret = oraw_input(to_utf8(prompt))
            rret = oraw_input(prompt)
            sys.stdout = owriter

            return rret

        __builtin__.raw_input = _sick_hack_raw_input
        try:
            cret = cmd.Cmd.cmdloop(self)
        finally:
            __builtin__.raw_input  = oraw_input
        return cret

    def asyncWrapper(self, command, args):
        """Execute a command which causes the inferior to run.
        """
        dbg0("asyncWrapper", command, args)
        command = "{} {}".format(command, args)
        dbg0("command", command)
        results = self.gdb.consoleCommand(command)


##########################
## Breakpoint commands ##
##########################

    def do_break(self, args, getSynopsis = False):
        """
        breakpoints
        NAME
            break -- Set breakpoint at specified line or function

        DESCRIPTION
            LOCATION may be a probe point, line number, function name, or "*" and an address.
            If a line number is specified, break at start of code for that line.
            If a function is specified, break at start of code for that function.
            If an address is specified, break at that exact address.
            With no LOCATION, uses current execution address of the selected
            stack frame.  This is useful for breaking on return to a stack frame.

            THREADNUM is the number from "info threads".
            CONDITION is a boolean expression.

            Multiple breakpoints at one place are permitted, and useful if their
            conditions are different.

            Do "help breakpoints" for info on other commands dealing with breakpoints.
        """
        parser = MyArgs(prog = "break", add_help = False)
        parser.add_argument("-t", "--temporary", action = "store_true", dest = "temporary")
        parser.add_argument("-h", "--hardware", action = "store_true", dest = "hw")
        parser.add_argument("-d", "--disabled", action = "store_true", dest = "disabled")
        parser.add_argument("-a", "--after", type = int, dest = "after")
        parser.add_argument("-p", "--probe", choices = ["generic", "stab"], dest = "probe", help = "Generic or SystemTap probe")
        parser.add_argument("location", nargs='?')
        # TODO add these back when we have optional subcommands working.
        #subparsers = parser.add_subparsers()
        #if_parser = subparsers.add_parser("if", add_help = False, help = "if CONDITION")
        #if_parser.add_argument("condition")
        #thread_parser = subparsers.add_parser("thread", add_help = False, help = "thread TID")
        #thread_parser.add_argument("tid", type = int)
        if getSynopsis:
            return parser.format_help()
        args = parser.parse_args(args.split())
        results = self.gdb._breakpoints.breakpointCreate(**vars(args))

    def do_info_breakpoints(self, args):
        results = self.gdb._breakpoints.list(args)
        if not len(results):
            return
        #
        # Print rows.
        #
        fmt = "{:<7} {:<14} {:<4} {:<3} {}"
        self._out(fmt.format("Num", "Type", "Disp", "Enb", "Where"))
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
                self._out(fmt.format(u["number"], u["type"], u["disp"], u["enabled"], location))
                try:
                    times = u["times"]
                    if times != "0":
                        self._out("        breakpoint already hit {} times".format(times))
                except KeyError:
                    pass
            except KeyError:
                #
                # Not a standalone breakpoint, just an overload of one.
                #
                location = "{} {}".format(u["addr"], u["at"])
                self._out(fmt.format(u["number"], "", "", u["enabled"], location))

###################
## Data commands ##
###################

    def do_call(self, args, getSynopsis = False):
        parser = MyArgs(prog = "call", add_help = False)
        parser.add_argument("expr")
        if getSynopsis:
            return parser.format_help()
        args = parser.parse_args(args.split())
        # TODO assign to local var
        self.gdb._data.evalute(**vars(args))

    def do_disassemble(self, args, getSynopsis = False):
        parser = MyArgs(prog = "disassemble", add_help = False)
        parser.add_argument("-s", "--start-addr", type = int)
        parser.add_argument("-e", "--end-addr", type = int)
        parser.add_argument("-f", "--filename")
        parser.add_argument("-l", "--linenum", type = int)
        parser.add_argument("-n", "--lines", type = int)
        # ["disassembly_only", "with_source", "with_opcodes", "all"]
        parser.add_argument("mode", type = int, choices = [ 0, 1, 2, 3 ])
        if getSynopsis:
            return parser.format_help()
        args = parser.parse_args(args.split())
        result = self.gdb._data.disassemble(**vars(args))
        for u in result:
            self._out(u[u'address'], u[u'inst'])

    def do_output(self, args, getSynopsis = False):
        parser = MyArgs(prog = "output", add_help = False)
        parser.add_argument("expr")
        if getSynopsis:
            return parser.format_help()
        args = parser.parse_args(args.split())
        self.gdb._data.evalute(**vars(args))

    def do_print(self, args, getSynopsis = False):
        parser = MyArgs(prog = "print", add_help = False)
        parser.add_argument("expr")
        if getSynopsis:
            return parser.format_help()
        args = parser.parse_args(args.split())
        # TODO assign to local var
        self.gdb._data.evalute(**vars(args))

    def do_print(self, args):
        """
        data
        NAME
            print -- Print value of expression EXP

        SYNOPSIS
            print EXP

        DESCRIPTION
            EXP can be any of:
            -       Inferior variables of the lexical environment of the selected
                stack frame, plus all those whose scope is global or an entire file.
            -       $NUM gets previous value number NUM.  $ and $$ are the last two
                values. $$NUM refers to NUM'th value back from the last one.
            -       Names starting with $ refer to registers (with the values they
                would have if the program were to return to the stack frame now
                selected, restoring all registers saved by frames farther in) or
                else to ...
            -       GDB "convenience" variables. Use assignment expressions to give
                values to convenience variables.
            -       {TYPE}ADREXP refers to a datum of data type TYPE, located at address
                ADREXP. @ is a binary operator for treating consecutive data objects
                anywhere in memory as an array.  FOO@NUM gives an array whose first
                element is FOO, whose second element is stored in the space following
                where FOO is stored, etc.  FOO must be an expression whose value
                resides in memory.
            -       Python expressions. In case of ambiguity between an inferior
                variable and a python variable, use the "gdb print" or "py print"
                commands.

            EXP may be preceded with /FMT, where FMT is a format letter
            but no count or size letter (see "x" command).

        EXAMPLES
            print main+1        Print inferior expression.
            print $1        Print previous value.
            print $getenv("HOME")    Print convenience function
            print gdb.PYTHONDIR    Print Python expression
        """
        try:
            #
            # Assume its an object known to GDB.
            #
            self.do_gdb("print " + args, name_errors = True)
        except NameError as e:
            #
            # Try a Python variable.
            #
            try:
                self._out(eval(args))
            except NameError as f:
                self._out("No GDB" + str(e)[2:-1] + ", and Python " + str(f))

    def do_info_registers(self, args, getSynopsis = False):
        parser = MyArgs(prog = "info registers", add_help = False)
        parser.add_argument("regName", nargs = "?")
        if getSynopsis:
            return parser.format_help()
        args = parser.parse_args(args.split())
        # TODO assign to local var
        results = self.gdb._data.listRegisterValues(**vars(args))
        #
        # Print rows.
        #
        for u in results:
            self._out(u[u'name'], u[u'value'])

    def do_info_all__registers(self, args, getSynopsis = False):
        parser = MyArgs(prog = "info all-registers", add_help = False)
        parser.add_argument("regName", nargs = "?")
        if getSynopsis:
            return parser.format_help()
        args = parser.parse_args(args.split())
        # TODO assign to local var
        results = self.gdb._data.listRegisterValues(**vars(args))
        #
        # Print rows.
        #
        for u in results:
            self._out(u[u'name'], u[u'value'])

    def do_x(self, args, getSynopsis = False):
        parser = MyArgs(prog = "x", add_help = False)
        parser.add_argument("address", type = int)
        parser.add_argument("word_format", choices = ["x", "d", "u", "o", "t", "a", "c", "f"])
        parser.add_argument("word_size", type = int)
        parser.add_argument("nr_rows", type = int)
        parser.add_argument("nr_cols", type = int)
        parser.add_argument("aschar", nargs="?", default = ".")
        parser.add_argument("-o", "--offset-bytes", type = int)
        if getSynopsis:
            return parser.format_help()
        args = parser.parse_args(args.split())
        # TODO assign to local var
        results = self.gdb._data.readMemory(**vars(args))
        for u in results:
            self._out(u[u'addr'], u[u'data'])

#####################
## Program control ##
#####################
    def do_advance(self, args):
        """
        running
        NAME
            advance -- Continue the program up to the given location (same form as args for break command)

        SYNOPSIS
            advance [PROBE_MODIFIER] [LOCATION] [thread THREADNUM] [if CONDITION]

        DESCRIPTION
            Continue the program up to the given location (same form as args for break command).
            Execution will also stop upon exit from the current stack frame.
        """
        self.asyncWrapper("advance", args)

    def do_continue(self, args):
        """
        running
        NAME
            continue -- Continue program being debugged

        SYNOPSIS
            continue [N|-a]

        DESCRIPTION
            Continue program being debugged, after signal or breakpoint.
            If proceeding from breakpoint, a number N may be used as an argument,
            which means to set the ignore count of that breakpoint to N - 1 (so that
            the breakpoint won't break until the Nth time it is reached).

            If non-stop mode is enabled, continue only the current thread,
            otherwise all the threads in the program are continued.  To
            continue all stopped threads in non-stop mode, use the -a option.
            Specifying -a and an ignore count simultaneously is an error.
        """
        self.gdb.miCommandExec("-exec-continue", args)

    def do_finish(self, args):
        """
        running
        NAME
            finish -- Execute until selected stack frame returns

        SYNOPSIS
            finish

        DESCRIPTION
            Execute until selected stack frame returns.
            Upon return, the value returned is printed and put in the value history.
        """
        self.gdb.miCommandExec("-exec-finish", args)

    def do_interrupt(self, args):
        self.gdb.miCommandExec("-exec-interrupt", args)

    def do_jump(self, args):
        """
        running
        NAME
            jump -- Continue program being debugged at specified line or address

        SYNOPSIS
            jump LINENUM|*ADDR

        DESCRIPTION
            Continue program being debugged at specified line or address.
            Give as argument either LINENUM or *ADDR, where ADDR is an expression
            for an address to start at.
        """
        self.asyncWrapper("jump", args)

    def do_kill(self, args):
        self.gdb.miCommandExec("-exec-abort", args)

    def do_next(self, args):
        """
        running
        NAME
            next -- Step program

        SYNOPSIS
            next [N]

        DESCRIPTION
            Step program, proceeding through subroutine calls.
            Like the "step" command as long as subroutine calls do not happen;
            when they do, the call is treated as one instruction.
            Argument N means do this N times (or till program stops for another reason).
        """
        self.gdb.miCommandExec("-exec-next", args)

    def do_nexti(self, args):
        """
        running
        NAME
            nexti -- Step one instruction

        SYNOPSIS
            nexti [N]

        DESCRIPTION
            Step one instruction, but proceed through subroutine calls.
            Argument N means do this N times (or till program stops for another reason).
        """
        self.gdb.miCommandExec("-exec-next-instruction", args)

    def do_return(self, args):
        self.gdb.miCommandExec("-exec-return", args)

    def do_reverse_continue(self, args):
        """
        running
        NAME
            reverse-continue -- Continue program being debugged but run it in reverse

        SYNOPSIS
            reverse-continue [N]

        DESCRIPTION
            Continue program being debugged but run it in reverse.
            If proceeding from breakpoint, a number N may be used as an argument,
            which means to set the ignore count of that breakpoint to N - 1 (so that
            the breakpoint won't break until the Nth time it is reached).
        """
        self.asyncWrapper("reverse-continue", args)

    def do_reverse_finish(self, args):
        """
        running
        NAME
            reverse-finish -- Execute backward until just before selected stack frame is called

        SYNOPSIS
            reverse-finish

        DESCRIPTION
            Execute backward until just before selected stack frame is called.
        """
        self.asyncWrapper("reverse-finish", args)

    def do_reverse_next(self, args):
        """
        running
        NAME
            reverse-next -- Step program backward

        SYNOPSIS
            reverse-next [N]

        DESCRIPTION
            Step program backward, proceeding through subroutine calls.
            Like the "reverse-step" command as long as subroutine calls do not happen;
            when they do, the call is treated as one instruction.
            Argument N means do this N times (or till program stops for another reason).
        """
        self.asyncWrapper("reverse-next", args)

    def do_reverse_nexti(self, args):
        """
        running
        NAME
            reverse-nexti -- Step backward one instruction

        SYNOPSIS
            reverse-nexti [N]

        DESCRIPTION
            Step backward one instruction, but proceed through called subroutines.
            Argument N means do this N times (or till program stops for another reason).
        """
        self.asyncWrapper("reverse-nexti", args)

    def do_reverse_step(self, args):
        """
        running
        NAME
            reverse-step -- Step program backward until it reaches the beginning of another source line

        SYNOPSIS
            reverse-step [N]

        DESCRIPTION
            Step program backward until it reaches the beginning of another source line.
            Argument N means do this N times (or till program stops for another reason).
        """
        self.asyncWrapper("reverse-step", args)

    def do_reverse_stepi(self, args):
        """
        running
        NAME
            reverse-stepi -- Step backward exactly one instruction

        SYNOPSIS
            reverse-stepi [N]

        DESCRIPTION
            Step backward exactly one instruction.
            Argument N means do this N times (or till program stops for another reason).
        """
        self.asyncWrapper("reverse-stepi", args)

    def do_run(self, args):
        """
        running
        NAME
            run -- Start debugged program

        SYNOPSIS
            run [ARGS]

        DESCRIPTION
            Start debugged program.  You may specify arguments to give it.
            Args may include "*", or "[...]"; they are expanded using "sh".
            Input and output redirection with ">", "<", or ">>" are also allowed.

            With no arguments, uses arguments last specified (with "run" or "set args").
            To cancel previous arguments and run with no arguments,
            use "set args" without arguments.
        """
        tty = self.gdb.startIoThread()
        self.gdb.miCommandOne("-inferior-tty-set {}".format(tty))
        if args:
            self.do_set_args(args)
        self.gdb.miCommandExec("-exec-run", args)

    def do_set_args(self, args):
        self.gdb.miCommandExec("-exec-arguments", args)

    def do_show_args(self, args):
        self.gdb.miCommandExec("-exec-show-arguments", args)

    def do_signal(self, args):
        """
        running
        NAME
            signal -- Continue program giving it signal specified by the argument

        SYNOPSIS
            signal N

        DESCRIPTION
            Continue program giving it signal specified by the argument.
            An argument of "0" means continue program without giving it a signal.
        """
        self.asyncWrapper("signal", args)

    def do_start(self, args):
        """
        running
        NAME
            start -- Run the debugged program until the beginning of the main procedure

        SYNOPSIS
            start [ARGS]

        DESCRIPTION
            Run the debugged program until the beginning of the main procedure.
            You may specify arguments to give to your program, just as with the
            "run" command.
        """
        results = self.gdb._breakpoints.breakpointCreate("main", temporary = True)
        if "pending" in results:
            results = self.gdb._breakpoints.breakpointDelete(results["number"])
            self._out("Cannot set breakpoint at 'main'")
            return
        self.do_run(args)

    def do_step(self, args):
        """
        running
        NAME
            step -- Step program until it reaches a different source line

        SYNOPSIS
            step [N]

        DESCRIPTION
            Step program until it reaches a different source line.
            Argument N means do this N times (or till program stops for another reason).
        """
        self.gdb.miCommandExec("-exec-step", args)

    def do_stepi(self, args):
        """
        running
        NAME
            stepi -- Step one instruction exactly

        SYNOPSIS
            stepi [N]

        DESCRIPTION
            Step one instruction exactly.
            Argument N means do this N times (or till program stops for another reason).
        """
        self.gdb.miCommandExec("-exec-step-instruction", args)

    def do_until(self, args):
        """
        running
        NAME
            until -- Execute until the program reaches a source line greater than the current

        SYNOPSIS
            until [PROBE_MODIFIER] [LOCATION] [thread THREADNUM] [if CONDITION]

        DESCRIPTION
            Execute until the program reaches a source line greater than the current
            or a specified location (same args as break command) within the current frame.
        """
        self.gdb.miCommandExec("-exec-until", args)

    def do_info_source(self, args):
        u = self.gdb._programControl.currentSource()
        self._out("Current source file is {}:{}".format(u["file"], u[u'line']))
        try:
            file = u["fullname"]
        except KeyError:
            file = u["file"]
        self._out("Located in {}".format(file))
        if u[u'macro-info'] != "0":
            self._out("Does include preprocessor macro info.")
        else:
            self._out("Does not include preprocessor macro info.")

    def do_info_sources(self, args):
        results = self.gdb._programControl.allSources()
        for u in results:
            try:
                file = u["fullname"]
            except KeyError:
                file = u["file"]
            self._out(file)

    def do_info_files(self, args):
        #self.gdb._programControl.execSections()
        self.gdb._programControl.symbolFiles()

    def do_info_target(self, args):
        self.do_info_files(args)

    def do_file(self, filename):
        self.gdb._programControl.setExecAndSymbols(filename)

    #def do_exec_file(self, filename):
    #    self.gdb._programControl.setExecOnly(filename)

    #def do_symbol_file(self, filename):
    #    self.gdb._programControl.setSymbolsOnly(filename)

####################
## Stack commands ##
####################

    def do_bt(self, args):
        results = self.gdb._stack.stackFrames(1)
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
                        self._out("#{}  {} in {} ()".format(u["level"], u["addr"], u["func"]))
                        continue
            self._out("#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], location))

    def do_backtrace(self, args):
        self.do_bt(args)

    def do_where(self, args):
        self.do_bt(args)

    #def do_depth(self, tid, maxFrames = None):

    def do_frame(self, args):
        if not args:
            self.do_info_frame(args)
        else:
            self.do_info_frame((1, 3))

    def do_info_frame(self, args):
        u = self.gdb._stack.frameInfo(1)
        self._out("#{}  {} in {} () from {}".format(u["level"], u["addr"], u["func"], u["from"]))

    def do_info_locals(self, args):
        #self.gdb._stack.stackArguments(1, 1)
        results = self.gdb._stack.frameVariables(1, 1, 8)
        for u in results:
            try:
                self._out("arg {} {} = {} = {}".format(u["arg"], u["name"], u["type"], u["value"]))
            except KeyError:
                try:
                    self._out("{} = {} = {}".format(u["name"], u["type"], u["value"]))
                except KeyError:
                    self._out("{} = {}".format(u["name"], u["value"]))

#####################
## Target commands ##
#####################
#'-target-attach'
#'-target-compare-sections'
#'-target-detach'
#'-target-disconnect'
#'-target-download'
#'-target-exec-status'
#'-target-list-available-targets'
#'-target-list-current-targets'
#'-target-list-parameters'
#'-target-list-parameters'

######################
## Thread commands ##
#####################
#'-thread-select'

    def do_info_threads(self, args):
        currentThread, results = self.gdb._threads.list(args)
        if not len(results):
            return
        #
        # Print rows.
        #
        fmt = "{:<1} {:<4} {:<37} {}"
        self._out(fmt.format(" ", "Id", "Target Id", "Where"))
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
            self._out(fmt.format(active, v["id"], name + v["target-id"], location))

######################
## General commands ##
######################
#'-enable-timings'
#'-environment-cd'
#'-environment-directory'
#'-environment-path'
#'-environment-pwd'
#'-gdb-exit'
#'-gdb-set'
#'-gdb-show'
#'-gdb-version'
#'-inferior-tty-set'
#'-inferior-tty-show'
#'-interpreter-exec'
#'-list-features'

    def do_apropos(self, args):
        """
        support
        NAME
            apropos -- Search for commands matching a REGEXP

        SYNOPSIS
            apropos REGEXP

        DESCRIPTION
            Type "apropos word" to search for commands related to "word".
        """

        def printAproposEntry(regexp, arg, indentation, prefix, keyword, apropos, clazz, function):
            """Dump the contents of the database as help text.
            Only leaf items which match the given regexp are emitted.
            """
            if regexp.search(keyword) or regexp.search(apropos):
                self._out("\t" + prefix + keyword + " -- " + apropos)

        #
        # We emit our help database, so that we can override GDB if needed.
        #
        if args == "":
            self._out("REGEXP string is empty")
            return
        self._out("LIST OF COMMANDS MATCHING '" + args + "'")
        self.commandDb.walk(printAproposEntry, re.compile(args, re.IGNORECASE), None, "\t")
        self._out("")

    def do_EOF(self, args):
        """
        alias
        NAME
            <Ctrl-D> -- Exit GDB.

        SYNOPSIS
            <Ctrl-D>

        DESCRIPTION
            Shortcut for "quit".
        """
        return True

    def do_quit(self, args):
        """
        support
        NAME
            quit -- Exit GDB.

        SYNOPSIS
            quit

        DESCRIPTION
            Exit the interpreter. Shortcut: <Ctrl-D>
        """
        return True

    def do_gdb(self, args):
        """
        support
        NAME
            gdb -- Execute a GDB command directly.

        SYNOPSIS
            gdb NATIVE-GDB-COMMAND

        DESCRIPTION
            The command is executed directly, bypassing any overrides in this wrapper.

        EXAMPLES
            gdb help        Get GDB's native help.
        """
        results = self.gdb.consoleCommand(args, True)
        for line in results:
            self._out(line)

    def do_help(self, args):
        """
        support
        NAME
            help -- Print list of commands

        SYNOPSIS
            help [COMMAND|COMMAND-CLASS]

        DESCRIPTION
            Type "help" followed by a class name for a list of commands in that class.
            Type "help all" for the list of all commands.
            Type "help" followed by command name for full documentation.
            Type "apropos word" to search for commands related to "word".
            Command name abbreviations are allowed if unambiguous.
        """

        def printManHeader(command, apropos, synopsis, description):
            if apropos:
                self._out("NAME\n\t" + command + " -- " + apropos)
            else:
                self._out("NAME\n\t" + command)
            if synopsis:
                self._out("\nSYNOPSIS\n\t" + synopsis.replace("\n", "\n\t"))
            if description:
                self._out("\n" + description)

        def printClassHelp(keyword):
            #
            # Now check if the user asked for class-based help.
            #
            if keyword == "all":
                #
                # We emit our help database, so that we can override GDB if needed.
                #
                self._out("LIST OF COMMANDS")
                self.commandDb.walk(printAproposEntry, "", None, "\t")
                self._out("")
                return True
            else:
                classes = [name for name in self.commandDb.classes_db if name.startswith(keyword)]
                if len(classes) == 1:
                    #
                    # Emit GDB help for the class.
                    #
                    error, helpText = self.gdb.consoleCommand("help " + classes[0], True)
                    apropos = helpText[0]
                    synopsis = None
                    for i in range(1, len(helpText)):
                        if helpText[i] == "":
                            #
                            # Skip the "List of commands"
                            #
                            helpText = helpText[i + 1:]
                            break
                        if synopsis:
                            synopsis = "\n\t".join((synopsis, helpText[i]))
                        else:
                            synopsis = helpText[i]
                    printManHeader(classes[0], apropos, synopsis, "LIST OF COMMANDS")
                    for line in helpText[2:]:
                        self._out("\t" + line)
                    return True
                elif len(classes) > 1:
                    message = "Ambiguous keyword: help"
                    self._out(" ".join((message, keywords[0], str(sorted(classes)))))
                    self._out("^".rjust(len(message) + 2))
                    return True
            return False

        def printAproposEntry(clazzPrefix, arg, indentation, prefix, keyword, apropos, clazz, function):
            """Dump the contents of the database as help text.
            Only leaf items which match the given classification prefix are emitted.
            """
            if clazz.startswith(clazzPrefix) :
                self._out(indentation + keyword + " -- " + apropos)

        keywords = args.split()
        if (keywords):
            #
            # First try to find command-specific help.
            #
            (matched, unmatched, completions, lastMatchedEntry) = self.commandDb.lookup(args)
            if unmatched:
                if isinstance(completions, dict):
                    if printClassHelp(keywords[0]):
                        return
                    #
                    # It was not a class-based request for help...
                    #
                    message = " ".join(("Keyword not found: help", matched)).rstrip()
                    self._out(" ".join((message, unmatched, str(sorted(completions.keys())))))
                    self._out("^".rjust(len(message) + 2))
                else:
                    message = " ".join(("Ambiguous keyword: help", matched)).rstrip()
                    self._out(" ".join((message, unmatched, str(sorted(completions)))))
                    self._out("^".rjust(len(message) + 2))
                return
            #
            # We got a match!
            #
            (oldApropos, oldLevel, oldClazz, oldFunction) = completions
            if oldFunction and oldFunction.__doc__:
                #
                # Emit help for our implementation if we have it.
                #
                helpText = oldFunction.__doc__.split("\n")
                synopsis = helpText[6].lstrip()
                if synopsis.startswith(matched):
                    helpText = [line[2:] for line in helpText[11:]]
                else:
                    helpText = [line[2:] for line in helpText[8:]]
                    synopsis = matched
            else:
                #
                # Emit help for the GDB implementation.
                #
                error, helpText = self.gdb.consoleCommand("help " + matched, True)
                if len(helpText) > 1 and (helpText[1].startswith(matched) or helpText[1].startswith("Usage:")):
                    synopsis = helpText[1]
                    helpText = ["\t" + line for line in helpText[2:]]
                elif len(helpText) > 2 and (helpText[2].startswith(matched) or helpText[2].startswith("Usage:")):
                    synopsis = helpText[2]
                    helpText = ["\t" + line for line in helpText[3:]]
                else:
                    helpText = ["\t" + line for line in helpText]
                    synopsis = matched
            #
            # If we have a dynamically generated synopsis, use it.
            #
            try:
                synopsis = oldFunction(None, getSynopsis = True)
                synopsis = synopsis[:-1]
            except TypeError:
                pass
            printManHeader(matched, oldApropos, synopsis, "DESCRIPTION")
            for line in helpText:
                self._out(line)
        else:
            #
            # Emit summary help from GDB.
            #
            helpText = self.gdb.consoleCommand("help", True)
            self._out("LIST OF CLASSES OF COMMANDS")
            for line in helpText[2:]:
                self._out("\t" + line)

    pythonShell = None
    def do_python(self, args):
        print("do_python(), calling enter", self.pythonShell)
        connectionFile = self.gdb._python.enter(args)
        if not self.pythonShell:
            self.pythonShell = IPythonConsoleShell(connection_file = connectionFile)
        self.pythonShell.interact()
        print("do_python(), pythonShell.interact done!")
        self.pythonShell.stop()
        self.gdb._python.exit()
        del self.pythonShell

#################################
## Fallthrough command handler ##
#################################

    def default(self, args):
        """
        Default command handler, for all commands not matched by a hand-crafted
        do_xxx() handler, and any special handlers.
        """

        def getenv(name):
            from ctypes import CDLL, cChar_p, stringAt
            libc = CDLL("libc.so.6")
            libc.getenv.argtypes = [cChar_p]
            libc.getenv.restype = cChar_p
            return libc.getenv(name)

        def expandEnvironmentVariables(line):
            """
            Fetch any environment variabled, i.e. $FOO or ${FOO}
            """
            regexp = re.compile(r"\${(\w+)}|\$(\w+)")
            match = regexp.search(line)
            while match:
                #
                # Extract the name of the environment variable.
                #
                envVar = match.group(1)
                if not envVar:
                    envVar = match.group(2)
                #
                # Substitute value.
                #
                envVar = getenv(envVar)
                if not envVar:
                    envVar = ""
                line = line[:match.start()] + envVar + line[match.end():]
                #
                # No recursive resolution for us, so continue from after the
                # substitution...
                #
                match = regexp.search(line, match.start() + len(envVar))
            return line

        #
        # Did we get a command?
        #
        (matched, unmatched, completions, lastMatchedEntry) = self.commandDb.lookup(args)
        if isinstance(completions, list):
            self._out("Ambiguous command \"{}\": {}.".format(unmatched, ", ".join(completions)))
            return
        elif isinstance(completions, tuple) and completions[1]:
            subcommands = completions[1]
            self._out("\"{}\" must be followed by the name of an {} command.\nList of {} subcommands:\n".format(matched, matched, matched))
            for k in sorted(subcommands.keys()):
                self._out("{} {} -- {}".format(matched, k, subcommands[k][0]))
            return
        #
        # Extract the arguments.
        #
        matchedFrags = matched.count(" ") + 1
        frags = args.split(None, matchedFrags);
        if matchedFrags >= len(frags):
            args = ""
        else:
            args = frags[matchedFrags]
            if matched in self.filesCommands:
                dbg0("is files command {}", matched)
                #
                # Does the command which takes files/paths? If so, expand
                # any embedded environment variables.
                #
                args = " ".join(expandEnvironmentVariables(args))
        try:
            func = getattr(self, "do_" + "_".join(matched.split()))
        except AttributeError:
            #
            # Invoke GDB...
            #
            self.do_gdb(args)
        else:
            func(args)

    def complete(self, text, state):
        """Use the command database to provide completions."""
        matchedKeywords, unmatchedKeyword, completions, lastMatchedEntry = self.commandDb.lookup(text)
        #self.stdout.write("=={}==\n".format((matched, unmatched, completions, lastMatchedEntry)))
        self.stdout.write("\n{}\n{}{}".format("\t".join(completions), self.prompt, text))
        return completions

    def completedefault(self, *ignored):
        self.stdout.write("completedefault {}".format(ignored))

    def completenames(self, text, *ignored):
        self.stdout.write("completenames {} {}".format(text, ignored))

if __name__ == "__main__":
    import sys

    class Test(QObject):
        def __init__(self, parent = None):
            gdb = Cli(["gdb"], print)
            gdb.do_file("/usr/local/bin/kate")
            gdb.do_start(None)
            gdb.do_break("QWidget::QWidget")
            gdb.do_info_breakpoints(None)
            gdb.do_continue(None)
            gdb.do_x("140737488346128 x 4 8 2") # 0x7fffffffdc10
            gdb.do_disassemble("-s 140737488346128 -e 140737488346140 0") # 0x7fffffffdc10
            gdb.cmdloop()

    app = QCoreApplication(sys.argv)
    foo = Test()
    #sys.exit(app.exec_())
