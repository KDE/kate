# -*- coding: utf-8 -*-
'''CMake helper plugin'''

#
# CMake helper plugin
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


import imp
import functools
import glob
import os
import subprocess
import sys
import types

from PyQt4 import uic
from PyQt4.QtGui import *

from PyKDE4.kdecore import i18nc
from PyKDE4.ktexteditor import KTextEditor

import kate

from libkatepate import ui, common
from libkatepate.autocomplete import AbstractCodeCompletionModel

from cmake_utils_settings import (CMAKE_BINARY, PROJECT_DIR, CMAKE_BINARY_DEFAULT, CMAKE_UTILS_SETTINGS_UI)
import cmake_help_parser


class CMakeCompletionModel(AbstractCodeCompletionModel):
    '''Completion model for CMake files'''
    # TODO Unit tests

    TITLE_AUTOCOMPLETION = i18nc('@label:listbox', 'CMake Auto Completion')
    MAX_DESCRIPTION = 100

    _cc_registrar_fn_name = 'register_command_completer'

    def __init__(self, app):
        super(CMakeCompletionModel, self).__init__(app)
        # Create an empty dict of command completers
        self.__command_completers = {}
        # Try to load all available particular command completers
        self._loadCompleters()

    def reset(self):
        '''Reset the model'''
        self.resultList = []

    def completionInvoked(self, view, word, invocationType):
        '''Completion has been inviked for given view and word'''
        # First of all check the document's MIME-type
        document = view.document()
        mimetype = document.mimeType()
        if mimetype != 'text/x-cmake':
            return

        print('CMakeCC [{}]: current word: "{}"'.format(mimetype, word))

        self.reset()                                        # Drop previously collected completions

        cursor = view.cursorPosition()
        # Try to detect completion context
        command, in_a_string, in_a_var, fn_params_range = self.find_current_context(document, cursor)
        print('CMakeCC: command="{}", in_a_string={}, in_a_var={}'.format(command, in_a_string, in_a_var))
        if fn_params_range.isValid():
            print('CMakeCC: params="{}"'.format(document.text(fn_params_range)))

        if in_a_var:
            # Try to complete a variable name
            self.TITLE_AUTOCOMPLETION = i18nc('@label:listbox', 'CMake Variables Completion')
            for var in cmake_help_parser.get_cmake_vars():
                self.resultList.append(
                    self.createItemAutoComplete(
                        text=var[0]
                      , category='constant'
                      , description=var[1]
                      )
                  )
            return
        if in_a_string:
            # If we a not in a variable expansion, but inside a string
            # there is nothing to complete!
            return
        # Try to complete a command
        if not command:
            # Try to complete a variable name
            self.TITLE_AUTOCOMPLETION = i18nc('@label:listbox', 'CMake Commands Completion')
            for cmd in cmake_help_parser.get_cmake_commands():
                self.resultList.append(
                    self.createItemAutoComplete(
                        text=cmd[0]
                      , category='function'
                      , description=cmd[1]
                      )
                  )
            return
        # Try to complete a particular command
        # 0) assemble parameter list preceded to the current completion position
        # TODO Unit tests
        text = document.text(fn_params_range).strip().split()
        found_string = False
        comp_list = []
        for tok in text:
            if tok.startswith('"'):
                comp_list.append(tok)
                found_string = True
            elif tok.endswith('"'):
                comp_list[-1] += ' ' + tok
                found_string = False
            elif found_string:
                comp_list[-1] += ' ' + tok
            else:
                comp_list.append(tok)
        # 1) call command completer
        self.try_complete_command(command, document, cursor, word, comp_list)


    def executeCompletionItem(self, document, word, row):
        # TODO Why this method is not called???
        print('CMakeCC: executeCompletionItem: ' + repr(word)+', row='+str(row))


    def find_current_context(self, document, cursor):
        '''Determinate current context under cursor'''
        # Parse whole document starting from a very first line!
        in_a_string = False
        in_a_command = False
        skip_next = False
        nested_var_level = 0
        command = None
        fn_params_start = None
        for current_line in range(0, cursor.line() + 1):
            line_str = document.line(current_line)
            prev = None
            should_count_pos = (current_line == cursor.line())
            for pos, c in enumerate(line_str):
                if should_count_pos and pos == cursor.column():
                    break
                if c == '#' and not in_a_string:
                    # TODO Syntax error if we r in a var expansion
                    break                                   # Ignore everything till the end of line
                if skip_next:                               # Should we skip current char?
                    skip_next = False                       # Yep!
                elif c == '\\':                             # Found a backslash:
                    skip_next = True                        #  skip next char!
                elif c == '"':                              # Found a quote char
                    in_a_string = not in_a_string           # Switch 'in a string' state
                    # TODO Syntax error if we r in a var expansion
                elif c == '{' and prev == '$':              # Looks like a variable expansion?
                    nested_var_level += 1                   # Yep, incrase var level
                elif c == '}':                              # End of a variable expansion
                    nested_var_level -= 1
                elif c == '(' and not in_a_string:          # Command params started
                    command = line_str[0:pos].strip()
                    # TODO Syntax error if we r in a var expansion
                    in_a_command = True
                    fn_params_start = KTextEditor.Cursor(current_line, pos + 1)
                elif c == ')' and not in_a_string:
                    # TODO Syntax error if we r in a var expansion
                    in_a_command = False
                    command = None
                    fn_params_start = None

                # TODO Handle generator expressions

                # Remember current char in a `prev' for next iteration
                prev = c
        if fn_params_start is not None:
            fn_params_range = KTextEditor.Range(fn_params_start, cursor)
        else:
            fn_params_range = KTextEditor.Range(-1, -1, -1, -1)
        return (command, in_a_string, nested_var_level != 0, fn_params_range)


    def _loadCompleters(self):
        # Load available command completers
        for directory in kate.applicationDirectories('cmake_utils/command_completers'):
            print('CMakeCC: directory={}'.format(directory))
            sys.path.append(directory)
            for completer in glob.glob(os.path.join(directory, '*_cc.py')):
                print('CMakeCC: completer={}'.format(completer))
                cc_name = os.path.basename(completer).split('.')[0]
                module = imp.load_source(cc_name, completer)
                if hasattr(module, self._cc_registrar_fn_name):
                    r = getattr(module, self._cc_registrar_fn_name)
                    if isinstance(r, types.FunctionType):
                        r(self.__command_completers)


    def try_complete_command(self, command, document, cursor, word, comp_list):
        '''Try to complete a command'''
        if command in self.__command_completers:
            if isinstance(self.__command_completers[command], types.FunctionType):
                # If a function registered as a completer, just call it...
                completions = self.__command_completers[command](document, cursor, word, comp_list)
            else:
                # Everything else, that is not a function, just pass to the generic completer
                completions = self._try_syntactic_completer(
                    self.__command_completers[command]
                  , document
                  , cursor
                  , word
                  , comp_list
                  )
        else:
            ui.popup(
                i18nc('@title:window', 'Attention')
              , i18nc('@info:tooltip', 'Sorry, no completion for <command>{}()</command>'.format(command))
              , 'dialog-information'
              )

            completions = []

        # Result of a completion function must be a list type
        if completions and isinstance(completions, list):
            self.TITLE_AUTOCOMPLETION = i18nc(
                '@label:listbox'
              , 'CMake <command>{}()</command> Completion'.format(command)
              )
            for c in completions:
                # If completion item is a tuple, we expect to have 2 items in it:
                # 0 is a 'text' and 1 is a 'description'
                if isinstance(c, tuple) and len(c) == 2:
                    self.resultList.append(
                        self.createItemAutoComplete(
                            text=c[0]
                        , description=c[1]
                        )
                    )
                else:
                    self.resultList.append(self.createItemAutoComplete(text=c))


    def _try_syntactic_completer(self, syntax, document, cursor, word, comp_list):
        print('CMakeCC: generic completer: syntax='+str(syntax))
        print('CMakeCC: generic completer: comp_list='+str(comp_list))
        result = []
        if isinstance(syntax, list):
            for sid, s in enumerate(syntax):
                (items, stop) = s.complete(document, cursor, word, comp_list, sid)
                if stop:
                    return items
                result += items
        else:
            (items, stop) = syntax.complete(document, cursor, word, comp_list)
            result = items
        print('CMakeCC: generic completer result={}'.format(result))
        # TODO sort | uniq
        return result


def _reset(*args, **kwargs):
    cmake_completation_model.reset()


# ----------------------------------------------------------
# Plugin configuration stuff
# ----------------------------------------------------------

class CMakeConfigWidget(QWidget):
    '''Configuration widget for this plugin.'''

    cmakeBinary = None
    projectBuildDir = None

    def __init__(self, parent=None, name=None):
        super(CMakeConfigWidget, self).__init__(parent)

        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), CMAKE_UTILS_SETTINGS_UI), self)

        self.reset();

    def apply(self):
        kate.configuration[CMAKE_BINARY] = self.cmakeBinary.text()
        try:
            cmake_help_parser.validate_cmake_executable(kate.configuration[CMAKE_BINARY])
        except ValueError as error:
            ui.popup(
                i18nc('@title:window', 'Error')
              , i18nc('@info:tooltip', 'CMake executable test run failed:<nl/><message>{}</message>'.format(error))
              , 'dialog-error'
              )
        # TODO Store the following for a current session!
        kate.configuration[PROJECT_DIR] = self.projectBuildDir.text()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if CMAKE_BINARY in kate.configuration:
            self.cmakeBinary.setText(kate.configuration[CMAKE_BINARY])

    def defaults(self):
        # TODO Dectect it!
        self.cmakeBinary.setText(CMAKE_BINARY_DEFAULT)
        self.projectBuildDir.setText('')


class CMakeConfigPage(kate.Kate.PluginConfigPage, QWidget):
    '''Kate configuration page for this plugin.'''
    def __init__(self, parent=None, name=None):
        super(CMakeConfigPage, self).__init__(parent, name)
        self.widget = CMakeConfigWidget(parent)
        lo = parent.layout()
        lo.addWidget(self.widget)

    def apply(self):
        self.widget.apply()

    def reset(self):
        self.widget.reset()

    def defaults(self):
        self.widget.defaults()
        self.changed.emit()


@kate.configPage(
    i18nc('@action:inmenu', 'CMake Helper Plugin')
  , i18nc('@title:group', 'CMake Helper Settings')
  , icon='preferences-other'
  )
def cmakeConfigPage(parent=None, name=None):
    return CMakeConfigPage(parent, name)

@kate.viewCreated
def createSignalAutocompleteCMake(view=None, *args, **kwargs):
    print('CMakeCC: Register completion model')
    try:
        view = view or kate.activeView()
        if view:
            cci = view.codeCompletionInterface()
            cci.registerCompletionModel(cmake_completation_model)
    except:
        print('CMake Helper Plugin: Unable to get an active view')
        pass


@kate.init
def init():
    pass
    # Set default value if not configured yet
    print('CMakeCC: enter init')
    if CMAKE_BINARY not in kate.configuration:
        kate.configuration[CMAKE_BINARY] = CMAKE_BINARY_DEFAULT
    if PROJECT_DIR not in kate.configuration:
        kate.configuration[PROJECT_DIR] = ''

    print('CMakeCC: init: cmakeBinary='.format(kate.configuration[CMAKE_BINARY]))

    # Initialize completion model
    createSignalAutocompleteCMake()


cmake_completation_model = CMakeCompletionModel(kate.application)
cmake_completation_model.modelReset.connect(_reset)


# kate: indent-width 4;
