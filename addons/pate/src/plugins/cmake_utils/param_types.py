# -*- coding: utf-8 -*-
'''Classes to define particular command syntax'''

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

import os
import sys
import types

from . import cmake_help_parser

ANY = 0
IDENTIFIER = 1
STRING = 2
FILE = 3                                                    # Try to complete a file relative current CMakeLists.txt
DIR = 4                                                     # Try to complete a dir relative current CMakeLists.txt
PACKAGE = 5                                                 # Try to complete a package to find
MODULE = 6                                                  # Try to complete a module to include
VERSION = 7
COMMAND = 8
COMMAND_NO_ARGS_KW = 9
PROPERTY = 10                                               # Try to complete a property name
POLICY = 11                                                 # Try to complete a policy number
ONE_OF = 12                                                 # Enumeration of completion itmes

ZERO_OR_MORE = '*'
ZERO_OR_ONE = '?'
ONE_OR_MORE = '+'


def _minimum_expected_params(params):
    '''Helper function to calculate minimum expected parameters to some option'''
    assert(isinstance(params, list))
    result = 0
    for p in params:
        assert(isinstance(p, tuple) and len(p) == 2)
        if isinstance(p[1], int):
            result += p[1]
        elif p[1] == ONE_OR_MORE:
            result += 1
        elif p[1] == ONE_OR_MORE or p[0] == ONE_OF:
            result += 1
    return result


class Option(object):
    '''Class to represent a single option possible parameters'''

    def __init__(self, name, count=1, args=None, description=None, exppos=None):
        self.name = name
        self.count = count
        assert(args is None or isinstance(args, list))
        self.args = args
        self.description = description
        assert(exppos is None or isinstance(exppos, int))
        self.exppos = exppos

    def complete(self, document, cursor, word, comp_list, sid):
        print(
            'CMakeCC: name={}, count={}, args={}, exppos={}'
              .format(self.name, self.count, self.args, self.exppos)
          )
        # Check if some particular position for this option is expected
        if self.exppos is not None and (len(comp_list) <= self.exppos or comp_list[self.exppos] != self.name):
            #print('CMakeCC: option={}, expected-position={}'.format(self.name, self.exppos))
            # Disallow to complete other options
            return ([(self.name, self.description)], True)

        # Check if option is optional or mandatory one w/ count == 1 and
        # it still not completed
        if self.name not in comp_list or not (self.count == ZERO_OR_ONE or self.count == 1):
            print('CMakeCC: option not in a comp_list or allowed to be more than once')
            return ([(self.name, self.description)], False)

        # Check if this option already in the completions list and must have some parameters
        if self.name in comp_list and self.args is not None:
            assert(0 < len(comp_list))

            result = []
            # If option allowed more than once append its name to the result
            if self.count == ZERO_OR_MORE or self.count == ONE_OR_MORE:
                print('CMakeCC: append self to result')
                result = [(self.name, self.description)]

            # Get actual parameters count after last occurrence of the option keyword
            opt_index = len(comp_list) - 1 - comp_list[::-1].index(self.name)
            opt_params_after = len(comp_list) - 1 - opt_index
            print('CMakeCC: opt_index='+repr(opt_index))
            print('CMakeCC: opt_params_after='+repr(opt_params_after))

            # Check if expected value exceeded
            if _minimum_expected_params(self.args) <= opt_params_after:
                print('CMakeCC: option args seem Ok')
                return (result, False)                      # Ok, allow other options to complete
            assert(opt_params_after < len(self.args))

            # Check expected argument types and try to complete them
            pp = self.args[opt_params_after]
            if pp[0] == FILE:
                # TODO Try to complete a file relative to the current CMakeLists.txt
                print('TODO: Try to complete a file relative to the current CMakeLists.txt')
            elif pp[0] == DIR:
                # TODO Try to complete a directory name relative to the current CMakeLists.txt
                print('TODO: Try to complete a directory name relative to the current CMakeLists.txt')
            elif pp[0] == PACKAGE:
                # TODO Try to complete a package name to find
                # NOTE Nowadays there is no options w/ such type of arg
                print('TODO: Try to complete a package name to find')
            elif pp[0] == MODULE:
                # TODO Try to complete a module name to include
                # NOTE Nowadays there is no options w/ such type of arg
                print('TODO: Try to complete a module name to include')
            elif pp[0] == PROPERTY:
                # TODO Try to complete a property name
                return (cmake_help_parser.get_cmake_properties(), True)
            elif pp[0] == ONE_OF:
                assert(isinstance(pp[1], list))
                return (result + pp[1], True)
            # Disallow to complete other options while not all required params
            # are entered for the current option
            return (result, True)
        return ([], False)

    def __str__(self):
        return self.name


class Value(object):
    '''Class to represent a value of given type w/o a preceding named option'''

    def __init__(self, args):
        assert(isinstance(args, list))
        self.args = args

    def complete(self, document, cursor, word, comp_list, sid):
        comp_idx = len(comp_list) - int(bool(document.text(word)))
        print('CMakeCC: value completer: args={}, comp_idx={}'.format(self.args, comp_idx))
        # If given value expected to be the first param and it's still not here
        if sid == 0 and comp_idx < _minimum_expected_params(self.args):
            if comp_idx < len(self.args):                    # Not all values are entered yet
                arg = self.args[comp_idx]
                if arg[0] == PACKAGE:
                    modules = cmake_help_parser.get_cmake_modules_list_all()
                    packages = [pkg[4:] for pkg in modules if pkg.startswith('Find')]
                    return (packages, True)
                if arg[0] == MODULE:
                    return (cmake_help_parser.get_cmake_modules_list_all(), True)
            # TODO Complete other arg types?
            # Do not even try to complete anything else
            return ([], True)
        if len(self.args) == 1 and self.args[0][0] == ONE_OF:
            # Show completion for enum type in case of unambiguous arg position
            return (self.args[0][1], False)
        # NOTE It doesn't cover all cases, but it is the best we can do here...
        return ([], False)

    def __str__(self):
        return '[value {}]'.format(self.args)


class MultiSignature(object):
    '''Class to define several syntaxes of the same command'''

    def __init__(self, dispatcher, *syntaxes):
        assert(isinstance(dispatcher, types.FunctionType))
        self.dispatcher = dispatcher
        self.syntaxes = syntaxes

    def complete(self, document, cursor, word, comp_list):
        result = []
        signature = self.dispatcher(comp_list)
        print('CMakeCC: MultiSignature: dispatch result: '+repr(signature))
        if signature is None:
            # It is still uncertain signature.
            # Collect all possible completions
            for syntax in self.syntaxes:
                for sid, s in enumerate(syntax):
                    (items, stop) = s.complete(document, cursor, word, comp_list, sid)
                    if stop:
                        return (items, True)
                    result += items
            return (result, False)
        elif isinstance(signature, int) and signature < len(self.syntaxes):
            syntax = self.syntaxes[signature]
            for sid, s in enumerate(syntax):
                (items, stop) = s.complete(document, cursor, word, comp_list, sid)
                print('CMakeCC: MSD: partial result={}, stop={}'.format(items, stop))
                if stop:
                    return (items, True)
                result += items
        elif isinstance(signature, list):
            result = signature
        print('CMakeCC: MSD: final result={}'.format(result))
        return (result, False)


class OneOf(object):
    '''Class to choose one of possible syntax'''

    def __init__(self, *syntaxes, exppos=None):
        self.syntaxes = syntaxes
        assert(exppos is None or isinstance(exppos, int))
        self.exppos = exppos

    def complete(self, document, cursor, word, comp_list, sid):
        result = []
        for syntax in self.syntaxes:
            (items, stop) = syntax.complete(document, cursor, word, comp_list, sid)
            if stop:
                return (items, True)
            result += items

        stop = False

        # If any of above returns empty list (meaning it's already completed)
        if len(result) != len(self.syntaxes):
            # Drop the result
            result = []
        elif self.exppos is not None and len(comp_list) == self.exppos:
            stop = True
        return (result, stop)

# kate: indent-width 4;
