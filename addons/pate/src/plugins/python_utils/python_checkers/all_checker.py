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

# This file originally was in this repository:
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/check_plugins/check_all_plugins.py>


import kate

from PyKDE4.kdecore import i18n
from libkatepate.errors import clearMarksOfError, showError

from python_checkers.utils import is_mymetype_python
from python_settings import (KATE_ACTIONS,
                             _PEP8_CHECK_WHEN_SAVE,
                             _PYFLAKES_CHECK_WHEN_SAVE,
                             _PARSECODE_CHECK_WHEN_SAVE,
                             DEFAULT_CHECK_PEP8_WHEN_SAVE,
                             DEFAULT_CHECK_PYFLAKES_WHEN_SAVE,
                             DEFAULT_PARSECODE_CHECK_WHEN_SAVE)


@kate.action
def checkAll(doc=None, excludes=None, exclude_all=False):
    """Check the syntax, pep8 and pyflakes errors of the document"""
    python_utils_conf = kate.configuration.root.get('python_utils', {})
    if not (not doc or (is_mymetype_python(doc) and
                        not doc.isModified())):
        return
    is_called = not bool(doc)
    from python_checkers.parse_checker import parseCode
    excludes = excludes or []
    currentDoc = doc or kate.activeDocument()
    mark_iface = currentDoc.markInterface()
    clearMarksOfError(currentDoc, mark_iface)
    if not exclude_all:
        if not 'parseCode' in excludes and (is_called or python_utils_conf.get(_PARSECODE_CHECK_WHEN_SAVE, DEFAULT_PARSECODE_CHECK_WHEN_SAVE)):
            parseCode.f(currentDoc, refresh=False)
        if not 'checkPyflakes' in excludes and (is_called or python_utils_conf.get(_PYFLAKES_CHECK_WHEN_SAVE, DEFAULT_CHECK_PYFLAKES_WHEN_SAVE)):
            try:
                from python_checkers.pyflakes_checker import checkPyflakes
                checkPyflakes.f(currentDoc, refresh=False)
            except ImportError:
                pass
        if not 'checkPep8' in excludes and (is_called or python_utils_conf.get(_PEP8_CHECK_WHEN_SAVE, DEFAULT_CHECK_PEP8_WHEN_SAVE)):
            from python_checkers.pep8_checker import checkPep8
            checkPep8.f(currentDoc, refresh=False)
    if not doc and currentDoc.isModified() and not excludes:
        showError(i18n('You must save the file first'))


@kate.init
@kate.viewCreated
def createSignalCheckDocument(view=None, *args, **kwargs):
    view = view or kate.activeView()
    doc = view.document()
    doc.modifiedChanged.connect(checkAll.f)

# kate: space-indent on; indent-width 4;
