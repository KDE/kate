# -*- coding: utf-8 -*-
'''CMake `cmake_policy()` command completer and registrar'''

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


_OPTIONS = ['VERSION', 'SET', 'GET', 'PUSH', 'POP']

def _cmake_policy_completer(document, cursor, word, comp_list):
    '''Function to complete individual CMake command
        (mostly for demo)
    '''
    # If nothing entered yet...
    if len(comp_list) == 0:
        # Return a list of available options
        return _OPTIONS
    # Ok, at least one item is here. Dispatch to concrete option handler
    elif comp_list[0] == 'SET':
        if len(comp_list) == 1:                             # Only option name?
            # Ok, lets complete the policy
            return cmake_help_parser.get_cmake_policies()
        elif len(comp_list) == 2:                           # last option remain
            return ['NEW', 'OLD']
        # Otherwise, syntax error and nothing to complete...
        return []
    elif comp_list[0] == 'GET':
        if len(comp_list) == 1:                             # Only option name?
            # Ok, lets complete the policy
            return cmake_help_parser.get_cmake_policies()
        # Only variable name remains, and we can't complete anything in that case
        return []
    elif comp_list[0] == 'VERSION':
        return []                                           # No way to complete anything else
    elif comp_list[0] == 'PUSH' or comp_list[0] == 'POP':
        return []                                           # Nothing to complete anymore
    # Probably partial option specified... Lets show variants!
    return _OPTIONS


def register_command_completer(completers):
    completers['cmake_policy'] = _cmake_policy_completer
