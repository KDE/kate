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

from . import settings


CMAKE_HELP_VARBATIM_TEXT_PADDING_SIZE = 9

_CMAKE_CACHE_FILE = 'CMakeCache.txt'
_HELP_TARGETS = [
    'command', 'module', 'policy', 'property', 'variable'
  ]
_CMAKE_HELP_SUBSECTION_DELIMITER_LENGTH = 78


class help_category:
    HELP_ITEM = -1
    COMMAND = _HELP_TARGETS.index('command')
    MODULE = _HELP_TARGETS.index('module')
    POLICY = _HELP_TARGETS.index('policy')
    PROPERTY = _HELP_TARGETS.index('property')
    VARIABLE = _HELP_TARGETS.index('variable')


def _form_cmake_aux_arg(paths):
    if paths:
        return ['-DCMAKE_MODULE_PATH=' + ';'.join(paths)]
    return []


def _get_aux_dirs_if_any():
    aux_dirs = settings.get_aux_dirs()
    if aux_dirs:
        return _form_cmake_aux_arg(aux_dirs)
    return []


def _parse_cmake_help(out):
    # NOTE Ignore the 1st line which is 'cmake version blah-blah' string
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
            #print('CMakeCC: append: ' + found_item + ' -- ' + line.strip())
            found_item = None
    return result


def _parse_cmake_help_2(out, subsection_first_word):
    # NOTE Ignore the 1st line which is 'cmake version blah-blah' string
    lines = out.decode('utf-8').splitlines()[1:]
    found_item = None
    result = {}
    partial = []
    subsection = None
    next_is_subsection = False
    for line in lines:
        if len(line.strip()) < 3:
            continue
        if line[0] == ' ' and line[1] == ' ' and line[2:].isidentifier():
            found_item = line.strip()
        elif found_item is not None:
            partial.append((found_item, line.strip()))
            #print('CMakeCC: [{}] append: {} -- {}'.format(subsection, found_item, line.strip()))
            found_item = None
        elif line.startswith(_CMAKE_HELP_SUBSECTION_DELIMITER_LENGTH * '-'):
            next_is_subsection = True
        elif next_is_subsection:
            if line.startswith(subsection_first_word):
                if subsection is not None:
                    result[subsection] = partial
                    partial = []
                subsection = line
            next_is_subsection = None
    if partial:
        assert(subsection is not None)
        result[subsection] = partial
    return result


def _spawn_cmake_grab_stdout(args, cmake_executable = None):
    if cmake_executable is None:
        cmake_utils_conf = kate.configuration.root.get('cmake_utils', {})

        if settings.CMAKE_BINARY in cmake_utils_conf:
            # TODO Set locale "C" before run cmake
            cmake_bin = cmake_utils_conf[settings.CMAKE_BINARY]
        else:
            raise ValueError(
                i18nc(
                    '@item:intext'
                  , 'CMake executable is not configured'
                  )
              )
    else:
        cmake_bin = cmake_executable

    #print('CMakeCC: going to spawn `{} {}`'.format(cmake_bin, ' '.join(args)))
    p = subprocess.Popen([cmake_bin] + args, stdout=subprocess.PIPE)
    out, err = p.communicate()
    if err:
        print('CMake helper: running `{} {}` finished with errors:\n{}'.format(cmake_bin, ' '.join(args), err))
        raise ValueError(
            i18nc(
                '@item:intext'
                , 'Running <command>%1</command> finished with errors:<nl/><message>%2</message>'
                , cmake_bin, err
                )
            )
    return out


def validate_cmake_executable(cmake_executable):
    # Make sure specified binary exists
    if os.path.isabs(cmake_executable) and os.path.exists(cmake_executable):
        out = _spawn_cmake_grab_stdout(['--version'], cmake_executable)
        lines = out.decode('utf-8').splitlines()
        # We expect a word 'cmake' in a very first line
        if not lines or lines[0].count('cmake') == 0:
            raise ValueError(
                i18nc(
                    '@item:intext'
                  , 'Specified CMake executable <command>%1</command> looks invalid', cmake_executable
                  )
              )
    else:
        raise ValueError(
            i18nc(
                '@item:intext'
              , 'Specified CMake executable <command>%1</command> not found', cmake_executable
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


@functools.lru_cache(maxsize=1)
def get_cmake_vars_list():
    out = _spawn_cmake_grab_stdout(['--help-variables'])
    return _parse_cmake_help_2(out, 'Variables ')


@functools.lru_cache(maxsize=1)
def get_cmake_commands_list():
    out = _spawn_cmake_grab_stdout(['--help-command-list'])
    return out.decode('utf-8').splitlines()[1:]


@functools.lru_cache(maxsize=1)
def get_cmake_deprecated_commands_list():
    out = _spawn_cmake_grab_stdout(['--help-compatcommands'])
    return _parse_cmake_help(out)


@functools.lru_cache(maxsize=1)
def get_cmake_policies_list():
    return [p[0] for p in get_cmake_policies()]


@functools.lru_cache(maxsize=1)
def get_cmake_properties_list():
    out = _spawn_cmake_grab_stdout(['--help-properties'])
    return _parse_cmake_help_2(out, 'Properties ')


@functools.lru_cache(maxsize=16)
def get_cmake_modules_list(aux_path = None):
    aux_dirs = []
    if aux_path is not None:
        if isinstance(aux_path, str) and aux_path:
            aux_dirs = _form_cmake_aux_arg([aux_path])
    out = _spawn_cmake_grab_stdout(aux_dirs + ['--help-module-list'])
    return out.decode('utf-8').splitlines()[1:]


@functools.lru_cache(maxsize=1)
def get_cmake_modules_list_all():
    out = _spawn_cmake_grab_stdout(_get_aux_dirs_if_any() + ['--help-module-list'])
    return out.decode('utf-8').splitlines()[1:]


@functools.lru_cache(maxsize=128)
def get_help_on(category, target):
    assert(0 <= category  and category < len(_HELP_TARGETS))
    aux_dirs = []
    if category == help_category.MODULE:
        aux_dirs = _get_aux_dirs_if_any()
    out = _spawn_cmake_grab_stdout(aux_dirs + ['--help-{}'.format(_HELP_TARGETS[category]), target])
    return out.decode('utf-8')


@functools.lru_cache(maxsize=32)
def get_cache_content(build_dir, is_advanced = False):
    if os.path.isdir(build_dir) and os.path.exists(os.path.join(build_dir, _CMAKE_CACHE_FILE)):
        cwd = os.path.abspath(os.curdir)                    # Store current directory
        os.chdir(build_dir)                                 # Change to the build dir
        out = _spawn_cmake_grab_stdout(['-N'] + ['-LAH'] if is_advanced else ['-LH'])
        # Back to the previous dir
        os.chdir(cwd)
        # Parse cache
        result = {}
        comment = None
        for line in out.decode('utf-8').splitlines():
            sline = line.strip()
            if sline.startswith('// '):
                comment = sline[3:]
            elif comment != None:
                assert(sline)                               # Line expected to be non empty
                key_end_idx= sline.index(':')
                type_end_idx = sline.index('=')
                result[sline[:key_end_idx]] = (
                    sline[type_end_idx + 1:]
                  , sline[key_end_idx + 1:type_end_idx]
                  , comment
                  )
                comment = None
        return result
    else:
        raise ValueError(
            i18nc(
                '@item:intext/plain'
                # BUG Using <filename> tag here leads to the following error:
                # (in cmake_utils.py @ _updateCacheView() around line 433)
                #
                # UnicodeEncodeError: 'ascii' codec can't encode character '\u2018' in position 57:
                # ordinal not in range(128)
                #
                # Cuz it transformed to UTF chars, and next call to i18nc() fails due assume
                # the string must be ASCII? Looks like i18nc() is not unicode aware...
                #
                # TODO WTF? What to do?
              , "Specified path %1 does not look like a CMake build directory", build_dir
              )
          )
