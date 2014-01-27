# -*- coding: utf-8 -*-
'''CMake `if()`, `elseif()` and `while()` command completers and registrar'''

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

import sys

from cmake_utils import cmake_help_parser
from cmake_utils.param_types import *


_FIRST_KW_OPTIONS = [
    'NOT', 'COMMAND', 'POLICY', 'TARGET', 'EXISTS', 'IS_DIRECTORY'
  , 'IS_SYMLINK', 'IS_ABSOLUTE', 'DEFINED'
  ]

_BINOP_KW_OPTIONS = [
    'AND', 'OR', 'IS_NEWER_THAN', 'LESS', 'GREATER', 'EQUAL', 'MATCHES'
  , 'STRLESS', 'STRGREATER', 'STREQUAL'
  , 'VERSION_LESS', 'VERSION_GREATER', 'VERSION_EQUAL'
  ]
def _conditions_completer(document, cursor, word, comp_list):
    '''Function to complete if() and elseif() commands'''

    comp_idx = len(comp_list) - int(bool(document.text(word)))
    # NOTE If comp_idx == len(comp_list), then user tries to
    # complete an empty word right after the last form `comp_list`

    print('CMakeCC: cond_cc: comp_idx={}, comp_list={}'.format(comp_idx, comp_list))

    first_arg_idx = int(1 <= comp_idx and comp_list[0] == 'NOT')

    # If nothing entered yet or just the only word...
    if comp_idx == 0 or comp_idx == first_arg_idx:
        # Return a list of available options that can occur at 0th position
        print('CMakeCC: cond_cc: return 1st kw-list')
        return _FIRST_KW_OPTIONS

    assert(1 <= len(comp_list))                             # At least one word in a list expected

    first_arg = comp_list[first_arg_idx]
    print('CMakeCC: cond_cc: first_arg_idx={}, first_arg={}'.format(first_arg_idx, first_arg))

    # Try to complete the word next to one of _FIRST_KW_OPTIONS
    if (first_arg_idx + 1) == comp_idx:
        if first_arg == 'POLICY':
            return cmake_help_parser.get_cmake_properties()

        if first_arg in _FIRST_KW_OPTIONS:
            # TODO Some of parameters can be completed,
            # like file/dir names
            return []

        # Dunno what 1st argument is, so lets guess it is left part
        # of some binary operation
        return _BINOP_KW_OPTIONS

    # Dunno what to guess...
    return []


def register_command_completer(completers):
    completers['if'] = _conditions_completer
    completers['elseif'] = _conditions_completer
    completers['while'] = _conditions_completer
