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

import kate

from libkatepate.menu import move_menu_submenu, separated_menu

PY_MENU = "Python"
PY_CHECKERS = "Checkers"

KATE_ACTIONS = {
    'insertIPDB': {'text': 'ipdb', 'shortcut': 'Ctrl+I',
                   'menu': PY_MENU, 'icon': 'tools-report-bug'},
    'insertInit': {'text': '__init__', 'shortcut': 'Ctrl+-',
                   'menu': PY_MENU, 'icon': None},
    'insertSuper': {'text': 'super', 'shortcut': 'Alt+-',
                    'menu': PY_MENU, 'icon': None},
    'callRecursive': {'text': 'call recursive', 'shortcut': 'Ctrl+Alt+-',
                    'menu': PY_MENU, 'icon': None},
    'checkAll': {'text': 'Check all', 'shortcut': 'Alt+5',
                 'menu': PY_CHECKERS, 'icon': None},
    'checkPyflakes': {'text': 'Pyflakes', 'shortcut': 'Alt+7',
                      'menu': PY_CHECKERS, 'icon': None},
    'parseCode': {'text': 'Parse code python', 'shortcut': 'Alt+6',
                  'menu': PY_CHECKERS, 'icon': None},
    'checkPep8': {'text': 'Pep8', 'shortcut': 'Alt+8',
                  'menu': PY_CHECKERS, 'icon': None},
}

PYTHON_AUTOCOMPLETE_ENABLED = True
CHECK_WHEN_SAVE = True
try:
    import pep8
    IGNORE_PEP8_ERRORS = pep8.DEFAULT_IGNORE.split(",")
except ImportError:
    IGNORE_PEP8_ERRORS = []
PYTHON_SPACES = 4
TEXT_INIT = """
    def __init__(self, *args, **kwargs):
        super(%s, self).__init__(*args, **kwargs)
"""
TEXT_SUPER = """%ssuper(%s, %s).%s(%s)\n"""
TEXT_RECURSIVE_CLASS = """%s%s.%s(%s)\n"""
TEXT_RECURSIVE_NO_CLASS = """%s%s(%s)\n"""


@kate.init
def move_menus():
    py_menu_slug = PY_MENU.lower()
    py_checkers_slug = PY_CHECKERS.lower()
    separated_menu(py_menu_slug)
    move_menu_submenu(py_menu_slug, py_checkers_slug)
