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
#
"""Replace gdb.execute with a wrapper that supports a timeout."""
from __future__ import print_function
import signal
import subprocess
import os

import gdb

def execute(timeout, command, from_tty = False, to_string = False):
    """Wrap gdb.execute with a timeout.

    timeout -- Millisecond resolution limit on the execution time. Note that
               the actual delay is implementation defined, and the implemented
               resolution may be as coarse as seconds. In any case, this is
               an approximate time only, intended mainly to ensure the script
               regains control.
    command -- See GDB docs.
    from_tty -- See GDB docs.
    to_string -- See GDB docs.
    Returns -- See GDB docs.
    """
    global __original_gdb_execute
    result = None
    if timeout > 0:
        # Using threading.Timer with os.kill(pid, signal.SIGINT) does not
        # work in GDB, hence the subprocess-based hack.
        pid = os.getpid()
        killer = subprocess.Popen("sleep " + str((timeout + 999)/1000) + " && pkill -INT -P " + str(pid) + " &", shell = True)
    result = __original_gdb_execute(command, from_tty, to_string)
    if timeout > 0:
        #
        # Cancel the timeout if needed and we can.
        #
        try:
            os.kill(killer.pid, signal.SIGTERM)
        except OSError:
            #
            # Too late...but that's fine too.
            #
            pass
    return result

#
# Support re-execution of this code. Ugly...
#
try:
    if __original_gdb_execute:
        pass
except NameError:
    __original_gdb_execute = gdb.execute
gdb.execute = execute
