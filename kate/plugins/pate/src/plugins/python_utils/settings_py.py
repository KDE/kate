# -*- coding: utf-8 -*-
# Copyright (c) 2013 by Pablo Martín <goinnn@gmail.com>
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


PY_MENU = "Python"
PY_CHECKERS = "Checkers"

KATE_ACTIONS = {
    'insertIPDB': {'text': 'ipdb', 'shortcut': 'Ctrl+I',
                   'menu': PY_MENU, 'icon': None},
    'insertInit': {'text': '__init__', 'shortcut': 'Ctrl+-',
                   'menu': PY_MENU, 'icon': None},
    'insertSuper': {'text': 'super', 'shortcut': 'Alt+-',
                    'menu': PY_MENU, 'icon': None},
    'callRecursive': {'text': 'call recursive', 'shortcut': 'Ctrl+Alt+-',
                    'menu': PY_MENU, 'icon': None},
    'checkAll': {'text': 'Check all', 'shortcut': 'Alt+5',
                 'menu': PY_CHECKERS, 'icon': None},
    'checkPyflakes': {'text': 'pyflakes', 'shortcut': 'Alt+7',
                      'menu': PY_CHECKERS, 'icon': None},
    'parseCode': {'text': 'Parse code python', 'shortcut': 'Alt+6',
                  'menu': PY_CHECKERS, 'icon': None},
    'checkPep8': {'text': 'Pep8', 'shortcut': 'Alt+8',
                  'menu': PY_CHECKERS, 'icon': None},
}

PYTHON_AUTOCOMPLETE_ENABLED = True
CHECK_WHEN_SAVE = True
IGNORE_PEP8_ERRORS = []
