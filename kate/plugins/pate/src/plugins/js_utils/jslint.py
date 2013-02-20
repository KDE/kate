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

from pyjslint import check_JSLint

from js_settings import KATE_ACTIONS, CHECK_WHEN_SAVE
from libkatepate.errors import (clearMarksOfError, hideOldPopUps,
                                showErrors, showOk)


pattern = re.compile(r"Lint at line (\d+) character (\d+): (.*)")


@kate.action(**KATE_ACTIONS['checkJslint'])
def checkJslint(currentDocument=None):
    """Check your js code with the jslint tool"""
    if not (not currentDocument or (is_mymetype_js(currentDocument) and
                                    not currentDocument.isModified())):
        return
    move_cursor = not currentDocument
    currentDocument = currentDocument or kate.activeDocument()
    mark_iface = currentDocument.markInterface()
    clearMarksOfError(currentDocument, mark_iface)
    hideOldPopUps()
    path = unicode(currentDocument.url().path())
    mark_key = '%s-jslint' % path

    text = unicode(currentDocument.text())
    errors = check_JSLint(text.encode('utf-8', 'ignore'))
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
        showOk("JSLint Ok")
        return

    showErrors('JSLint Errors:',
                errors_to_show,
                mark_key, currentDocument,
                move_cursor=move_cursor)


def is_mymetype_js(doc, text_plain=False):
    mimetype = unicode(doc.mimeType())
    if mimetype == 'application/javascript':
        return True
    elif mimetype == 'text/plain' and text_plain:
        return True
    return False


@kate.init
@kate.viewCreated
def createSignalCheckDocument(view=None, *args, **kwargs):
    if not CHECK_WHEN_SAVE:
        return
    view = view or kate.activeView()
    doc = view.document()
    doc.modifiedChanged.connect(checkJslint.f)
