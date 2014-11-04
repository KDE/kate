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

from PyKDE4.kdecore import i18n
from libkatepate.menu import move_menu_submenu, separated_menu

PY_MENU = "Python"
PY_CHECKERS = i18n("Checkers")

KATE_ACTIONS = {
    'insertIPDB': {'text': i18n('ipdb'), 'shortcut': 'Ctrl+I',
                   'menu': PY_MENU, 'icon': 'tools-report-bug'},
    'insertInit': {'text': i18n('__init__ method'), 'shortcut': 'Ctrl+,',
                   'menu': PY_MENU, 'icon': None},
    'insertSuper': {'text': i18n('call super'), 'shortcut': 'Alt+,',
                    'menu': PY_MENU, 'icon': None},
    'callRecursive': {'text': i18n('call recursive'), 'shortcut': 'Ctrl+Alt+,',
                      'menu': PY_MENU, 'icon': None},
    'checkAll': {'text': i18n('Check all'), 'shortcut': 'Alt+5',
                 'menu': PY_CHECKERS, 'icon': None},
    'checkPyflakes': {'text': i18n('Pyflakes'), 'shortcut': 'Alt+7',
                      'menu': PY_CHECKERS, 'icon': None},
    'parseCode': {'text': i18n('Syntax Errors'), 'shortcut': 'Alt+6',
                  'menu': PY_CHECKERS, 'icon': None},
    'checkPep8': {'text': i18n('Pep8'), 'shortcut': 'Alt+8',
                  'menu': PY_CHECKERS, 'icon': None},
}

KATE_CONFIG = {'name': 'python_utils',
               'fullName': i18n('Python Utils'),
               'icon': 'text-x-python'}

_PEP8_CHECK_WHEN_SAVE = 'PythonUtils:checkPEP8WhenSave'
_IGNORE_PEP8_ERRORS = 'PythonUtils:ignorePEP8Errors'
_PYFLAKES_CHECK_WHEN_SAVE = 'PythonUtils:checkPyFlakesWhenSave'
_PARSECODE_CHECK_WHEN_SAVE = 'PythonUtils:checkParseCode'
_IPDB_SNIPPET = 'PythonUtils:ipdbSnippet'

DEFAULT_CHECK_PEP8_WHEN_SAVE = True
try:
    import pep8
    DEFAULT_IGNORE_PEP8_ERRORS = pep8.DEFAULT_IGNORE
except ImportError:
    DEFAULT_IGNORE_PEP8_ERRORS = ''
DEFAULT_CHECK_PYFLAKES_WHEN_SAVE = True
DEFAULT_PARSECODE_CHECK_WHEN_SAVE = True
DEFAULT_IPDB_SNIPPET = "import ipdb; ipdb.set_trace()"

TEXT_INIT = """\tdef __init__(self, *args, **kwargs):
\t\tsuper(%s, self).__init__(*args, **kwargs)
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


# kate: space-indent on; indent-width 4;
