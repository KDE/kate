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


import re

import kate

from PyKDE4.kdecore import i18n

from pyjslint import check_JSLint

from js_settings import (KATE_ACTIONS, SETTING_LINT_ON_SAVE)
from libkatepate.errors import (clearMarksOfError, hideOldPopUps,
                                showErrors, showOk)


pattern = re.compile(r"Lint at line (\d+) character (\d+): (.*)")


def lint_js(document, move_cursor=False):
    """Check your js code with the jslint tool"""
    mark_iface = document.markInterface()
    clearMarksOfError(document, mark_iface)
    hideOldPopUps()
    path = document.url().path()
    mark_key = '%s-jslint' % path

    text = document.text()
    errors = check_JSLint(text)
    errors_to_show = []

    # Prepare errors found for painting
    for error in errors:
        matches = pattern.search(error)
        if matches:
            errors_to_show.append({
                "message": matches.groups()[2],
                "line": int(matches.groups()[0]),
                "column": int(matches.groups()[1]) + 1,
            })

    if len(errors_to_show) == 0:
        showOk(i18n("JSLint Ok"))
        return

    showErrors(i18n('JSLint Errors:'),
               errors_to_show,
               mark_key, document,
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
    print(view, args, kwargs)
    doc = view.document() if view else kate.activeDocument()
    doc.modifiedChanged.connect(lint_on_save)

# kate: space-indent on; indent-width 4;
