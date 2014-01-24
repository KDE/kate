# -*- coding: utf-8 -*-
#
''' Helpers to handle parameters in user defined expansion functions. '''
#
# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2010-2013 Alex Turbov <i.zaufi@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import re

_TEMPLATE_SEQ_PARAMS = re.compile('([^0-9]+)([0-9]+)\.\.([0-9]+)')

_FUSSY_FALSE_VALUES = [False, '0', 'n', 'N', 'f', 'F', 'false', 'False', 'No', 'no']
_FUSSY_TRUE_VALUES = [True, '1', 'y', 'Y', 't', 'T', 'true', 'True', 'Yes', 'yes']


def try_expand_params_seq(arg):
    '''
        Check if given `arg` contains a numeric range at the end,
        generate required count of parameters to the result list,
        or just add a single (initial) arg as a result.

        Example:
            try_expand_params_seq('U')      # ['U']
            try_expand_params_seq('T0..4')  # ['T0', 'T1', 'T2', 'T3', 'T4']
    '''
    result = []
    m = _TEMPLATE_SEQ_PARAMS.search(arg)
    if m:
        start = int(m.group(2))
        end = int(m.group(3))
        param = m.group(1)
        for i in range(start, end + 1):
            result.append('{}{}'.format(param, i))
    else:
        result.append(arg)

    return result


def sum_arg_lengths(args):
    return sum(len(s) for s in args)


def looks_like_false(arg):
    return arg in _FUSSY_FALSE_VALUES


def looks_like_true(arg):
    return arg in _FUSSY_TRUE_VALUES
