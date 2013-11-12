# -*- coding: utf-8 -*-
# Copyright (c) 2013 by Pablo Martín <goinnn@gmail.com> and
# Alejandro Blanco <alejandro.b.e@gmail.com>
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

# This file originally was in this repository:
# <https://github.com/goinnn/Kate-plugins/blob/master/kate_plugins/jste_plugins/jslint_plugins.py>


"""
format of errors:
{
    line      : The line (relative to 0) at which the lint was found
    character : The character (relative to 0) at which the lint was found
    reason    : The problem
    evidence  : The text line in which the problem occurred
    raw       : The raw message before the details were inserted
    a         : The first detail
    b         : The second detail
    c         : The third detail
    d         : The fourth detail
}
"""


import os.path as p

import kate

from PyKDE4.kdecore import i18nc

from js_settings import SETTING_LINT_ON_SAVE, SETTING_LINTER
from js_engine import PyJSEngine, JSModule
from libkatepate.errors import (clearMarksOfError, hideOldPopUps,
                                showErrors, showOk)


JS_ENGINE = PyJSEngine()

JS_LINT_PATH = p.join(p.dirname(__file__), 'fulljslint.js')
JS_HINT_PATH = p.join(p.dirname(__file__), 'jshint.js')

LINTERS = {  # keys() == SETTING_LINTER.choices
    'JSLint': JSModule(JS_ENGINE, JS_LINT_PATH, 'JSLINT'),
    'JSHint': JSModule(JS_ENGINE, JS_HINT_PATH, 'JSHINT'),
}

def lint_js(document, move_cursor=False):
    """Check your js code with the jslint tool"""
    mark_iface = document.markInterface()
    clearMarksOfError(document, mark_iface)
    hideOldPopUps()

    linter_name = SETTING_LINTER.choices[SETTING_LINTER.lookup()]  # lookup() gives index of choices
    linter = LINTERS[linter_name]

    ok = linter(document.text(), {})
    if ok:
        showOk(i18nc('@info:status', '<application>%1</application> OK', linter_name))
        return

    errors = [error for error in linter['errors'] if error]  # sometimes None

    # Prepare errors found for painting
    for error in errors:
        error['message'] = error.pop('reason')  # rename since showErrors has 'message' hardcoded
        error.pop('raw', None)  # Only reason, line, and character are always there
        error.pop('a', None)
        error.pop('b', None)
        error.pop('c', None)
        error.pop('d', None)

    mark_key = '{}-{}'.format(document.url().path(), linter_name)

    showErrors(i18nc('@info:status', '<application>%1</application> Errors:', linter_name),
               errors,
               mark_key, document,
               key_column='character',
               move_cursor=move_cursor)


def lint_on_save(document):
    """Tests for multiple Conditions and lints if they are met"""
    if (not document.isModified() and
        document.mimeType() == 'application/javascript' and
        SETTING_LINT_ON_SAVE.lookup()):
        lint_js(document)


def init_js_lint(view=None):
    doc = view.document() if view else kate.activeDocument()
    doc.modifiedChanged.connect(lint_on_save)

# kate: space-indent on; indent-width 4;
