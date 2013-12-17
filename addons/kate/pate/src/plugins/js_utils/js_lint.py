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

from __future__ import absolute_import

from os import path as p
from functools import partial

import kate

from PyKDE4.kdecore import i18nc, KGlobal, KUrl
from PyKDE4.kdeui import KMessageBox
from PyKDE4.kio import KIO

from .js_settings import SETTING_LINT_ON_SAVE, SETTING_LINTER
from .js_engine import PyJSEngine, JSModule
from libkatepate.errors import clearMarksOfError, showErrors, showOk, showError


JS_ENGINE = PyJSEngine()

# If doug crockford wasn’t special, we could do e.g.:
# LINTERS = { 'JSLint': JSModule(JS_ENGINE, p.join(p.dirname(__file__), 'jslint.js'), 'JSLINT') }

LINTERS = {}  # keys() == SETTING_LINTER.choices

_DOUG_LICENSE = 'The Software shall be used for Good, not Evil.'
NEEDS_LICENSE = {
    'JSLint': (_DOUG_LICENSE, 'JSLINT', 'https://raw.github.com/douglascrockford/JSLint/master/jslint.js'),
    'JSHint': (_DOUG_LICENSE, 'JSHINT', 'https://raw.github.com/jshint/jshint/2.x/dist/jshint.js'),
}

CACHE_DIR = KGlobal.dirs().locateLocal('appdata', 'pate/js_utils/', True)  # trailing slash necessary


def license_accepted(license):
    """asks to accept a license"""
    return KMessageBox.Yes == KMessageBox.warningYesNo(kate.mainWindow(),
        i18nc('@info:status', '''<p>
            Additionally to free software licenses like GPL and MIT,
            this functionality requires you to accept the following conditions:
            </p><p>%1</p><p>
            Do you want to accept and download the functionality?
            </p>''', license),
        i18nc('@title:window', 'Accept license?'))


def get_linter(linter_name, callback):
    """tries to retrieve a linter and calls `callback` on it on success"""
    if linter_name in LINTERS:
        callback(LINTERS[linter_name])
        return

    if linter_name not in NEEDS_LICENSE:
        showError(i18nc('@info:status', 'No acceptable linter named %1!', linter_name))
        return

    license, objname, url = NEEDS_LICENSE[linter_name]
    cache_path = p.join(CACHE_DIR, linter_name + '.js')

    def success():
        """store newly created linter and “return” it"""
        LINTERS[linter_name] = JSModule(JS_ENGINE, cache_path, objname)
        callback(LINTERS[linter_name])

    if p.exists(cache_path):
        success()
        return

    # the user doesn’t have the file. ask to accept its license
    if not license_accepted(license):
        return

    download = KIO.file_copy(KUrl(url), KUrl.fromPath(cache_path))
    @download.result.connect
    def _call(job):
        if job.error():
            showError(i18nc('@info:status', 'Download failed'))
        else:
            success()
    download.start()


def lint_js(document, move_cursor=False):
    """Check your js code with the jslint tool"""
    mark_iface = document.markInterface()
    clearMarksOfError(document, mark_iface)

    linter_name = SETTING_LINTER.choices[SETTING_LINTER.lookup()]  # lookup() gives index of choices
    get_linter(linter_name, partial(_lint, document, move_cursor, linter_name))


def _lint(document, move_cursor, linter_name, linter):
    """extracted part of lint_js that has to be called after the linter is ready"""
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
