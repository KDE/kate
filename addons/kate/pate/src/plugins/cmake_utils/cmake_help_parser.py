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
import os
import subprocess

from PyKDE4.kdecore import i18nc

import kate

from cmake_utils_settings import (CMAKE_BINARY, PROJECT_DIR, CMAKE_BINARY_DEFAULT)


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


def _spawn_cmake_grab_stdout(args, cmake_executable = None):
    if cmake_executable is None:
        cmake_utils_conf = kate.configuration.root.get('cmake_utils', {})

        if CMAKE_BINARY in cmake_utils_conf:
            cmake_bin = cmake_utils_conf[CMAKE_BINARY]
        else:
            raise ValueError(
                i18nc(
                    '@info:tooltip'
                  , 'CMake executable is not configured'
                  )
              )
    else:
        cmake_bin = cmake_executable

    p = subprocess.Popen([cmake_bin] + args, stdout=subprocess.PIPE)
    out, err = p.communicate()
    if err:
        print('CMake helper: running `{}` finished with errors:\n{}'.format(cmake_bin, err))
        raise ValueError(
            i18nc(
                '@info:tooltip'
                , 'Running <command>{}</command> finished with errors:<nl/>{}'.format(cmake_bin, err)
                )
            )
    return out


def validate_cmake_executable(cmake_executable):
    # Make sure specified binary exists
    print('CMakeCC: validate cmake: bin={}'.format(cmake_executable))
    print('CMakeCC: validate cmake: os.path.isabs(cmake_executable)={}'.format(os.path.isabs(cmake_executable)))
    print('CMakeCC: validate cmake: os.path.exists(cmake_executable)={}'.format(os.path.exists(cmake_executable)))
    if os.path.isabs(cmake_executable) and os.path.exists(cmake_executable):
        out = _spawn_cmake_grab_stdout(['--version'], cmake_executable)
        lines = out.decode('utf-8').splitlines()
        # We expect a word 'cmake' in a very first line
        if not lines or lines[0].count('cmake') == 0:
            raise ValueError(
                i18nc(
                    '@info:tooltip'
                  , 'Specified CMake executable <command>{}</command> looks invalid'.format(cmake_executable)
                  )
              )
    else:
        raise ValueError(
            i18nc(
                '@info:tooltip'
              , 'Specified CMake executable <command>{}</command> not found'.format(cmake_executable)
              )
          )


@functools.lru_cache(maxsize=1)
def get_cmake_vars():
    out = _spawn_cmake_grab_stdout(['--help-variables'])
    return _parse_cmake_help(out)


@functools.lru_cache(maxsize=1)
def get_cmake_commands():
    out = _spawn_cmake_grab_stdout(['--help-commands'])
    return _parse_cmake_help(out)


@functools.lru_cache(maxsize=1)
def get_cmake_policies():
    out = _spawn_cmake_grab_stdout(['--help-policies'])
    return _parse_cmake_help(out)


@functools.lru_cache(maxsize=1)
def get_cmake_properties():
    out = _spawn_cmake_grab_stdout(['--help-properties'])
    return _parse_cmake_help(out)


# kate: indent-width 4;
