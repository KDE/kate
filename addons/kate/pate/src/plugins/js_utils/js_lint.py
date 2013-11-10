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
import re

import kate

from PyQt4.QtScript import QScriptEngine
from PyKDE4.kdecore import i18n

from js_settings import (KATE_ACTIONS, SETTING_LINT_ON_SAVE)
from js_engine import PyJSEngine, JSModule
from libkatepate.errors import (clearMarksOfError, hideOldPopUps,
                                showErrors, showOk)


JS_ENGINE = PyJSEngine()

JS_LINT_PATH = p.join(p.dirname(__file__), 'fulljslint.js')
JS_LINTER = JSModule(JS_ENGINE, JS_LINT_PATH, 'JSLINT')


def lint_js(document, move_cursor=False):
    """Check your js code with the jslint tool"""
    mark_iface = document.markInterface()
    clearMarksOfError(document, mark_iface)
    hideOldPopUps()
    mark_key = '%s-jslint' % document.url().path()

    ok = JS_LINTER(document.text(), {})
    if ok:
        showOk(i18n("JSLint Ok"))
        return

    errors = [error for error in JS_LINTER['errors'] if error]  # sometimes None

    # Prepare errors found for painting
    for error in errors:
        error['message'] = error.pop('reason')  # rename since showErrors has 'message' hardcoded
        error.pop('raw', None)  # Only reason, line, and character are always there
        error.pop('a', None)
        error.pop('b', None)
        error.pop('c', None)
        error.pop('d', None)

    showErrors(i18n('JSLint Errors:'),
               errors,
               mark_key, document,
               key_column='character',
               move_cursor=move_cursor)


@kate.action(**KATE_ACTIONS.lint_JS)
def lint_js_action():
    """Lints the active document"""
    lint_js(kate.activeDocument(), True)


def lint_on_save(document):
    """Tests for multiple Conditions and lints if they are met"""
    if (not document.isModified() and
        document.mimeType() == 'application/javascript' and
        SETTING_LINT_ON_SAVE.lookup()):
        lint_js(document)


@kate.init
@kate.viewCreated
def init(view=None):
    doc = view.document() if view else kate.activeDocument()
    doc.modifiedChanged.connect(lint_on_save)

# kate: space-indent on; indent-width 4;
