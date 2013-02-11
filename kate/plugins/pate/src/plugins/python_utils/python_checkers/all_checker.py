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

# This plugin originally was in this repository:
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/check_plugins/check_all_plugins.py>


import kate

from python_checkers.utils import is_mymetype_python
from python_settings import KATE_ACTIONS, CHECK_WHEN_SAVE


def clearMarksOfError(doc, mark_iface):
    for line in range(doc.lines()):
        if mark_iface.mark(line) == mark_iface.Error:
            mark_iface.removeMark(line, mark_iface.Error)


def hideOldPopUps():
    mainWindow = kate.mainWindow()
    popups = kate.gui.TimeoutPassivePopup.popups.get(mainWindow, []) or []
    for popup in popups:
        popup.timer.stop()
        popup.hide()
        popup.setFixedHeight(0)
        popup.adjustSize()
        popup.originalHeight = popup.height()
        popup.offsetBottom = 0
        popup.move(0, 0)


@kate.action(**KATE_ACTIONS['checkAll'])
def checkAll(doc=None, excludes=None, exclude_all=False):
    """Check the syntax, pep8 and pyflakes errors of the document"""
    if not (not doc or (is_mymetype_python(doc) and
                        not doc.isModified())):
        return
    from python_checkers.parse_checker import parseCode
    excludes = excludes or []
    currentDoc = doc or kate.activeDocument()
    mark_iface = currentDoc.markInterface()
    clearMarksOfError(currentDoc, mark_iface)
    hideOldPopUps()
    if not exclude_all:
        if not 'parseCode' in excludes:
            parseCode.f(currentDoc, refresh=False)
        if not 'checkPyflakes' in excludes:
            try:
                from python_checkers.pyflakes_checker import checkPyflakes
                checkPyflakes.f(currentDoc, refresh=False)
            except ImportError:
                pass
        if not 'checkPep8' in excludes:
            try:
                from python_checkers.pep8_checker import checkPep8
                checkPep8.f(currentDoc, refresh=False)
            except ImportError:
                pass
    if not doc and currentDoc.isModified() and not excludes:
        kate.gui.popup('You must save the file first', 3,
                        icon='dialog-warning', minTextWidth=200)


@kate.init
@kate.viewCreated
def createSignalCheckDocument(view=None, *args, **kwargs):
    if not CHECK_WHEN_SAVE:
        return
    view = view or kate.activeView()
    doc = view.document()
    doc.modifiedChanged.connect(checkAll.f)
