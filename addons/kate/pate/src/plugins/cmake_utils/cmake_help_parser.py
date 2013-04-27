# -*- coding: utf-8 -*-
'''Helpers to parse CMake --help-xxx output'''

#
# Copyright 2013 by Alex Turbov <i.zaufi@gmail.com>
#
# This software is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this software.  If not, see <http://www.gnu.org/licenses/>.


import functools
import subprocess


def _parse_cmake_help(out):
    # NOTE Ignore the 1st line wich is 'cmake version blah-blah' string
    lines = out.decode('utf-8').splitlines()[1:]
    found_item = None
    result = []
    for line in lines:
        if len(line.strip()) < 3:
            continue
        if line[0] == ' ' and line[1] == ' ' and line[2:].isidentifier():
            found_item = line.strip()
        elif found_item is not None:
            result.append((found_item, line.strip()))
            print('CMakeCC: append: ' + found_item + ' -- ' + line.strip())
            found_item = None
    return result


def _spawn_cmake_grab_stdout(args):
    # TODO Find `cmake` (at program start) before spawn it!
    p = subprocess.Popen(["/usr/bin/cmake"] + args, stdout=subprocess.PIPE)
    out, err = p.communicate()
    if err:
        print('CMake helper: running `{}` finished with errors:\n{}'.format('/usr/bin/cmake', err))
    return out


@functools.lru_cache(maxsize=1)
def get_cmake_vars():
    out = _spawn_cmake_grab_stdout(["--help-variables"])
    return _parse_cmake_help(out)


@functools.lru_cache(maxsize=1)
def get_cmake_commands():
    out = _spawn_cmake_grab_stdout(["--help-commands"])
    return _parse_cmake_help(out)


@functools.lru_cache(maxsize=1)
def get_cmake_policies():
    out = _spawn_cmake_grab_stdout(["--help-policies"])
    return _parse_cmake_help(out)

@functools.lru_cache(maxsize=1)
def get_cmake_properties():
    out = _spawn_cmake_grab_stdout(["--help-properties"])
    return _parse_cmake_help(out)


# kate: indent-width 4;
