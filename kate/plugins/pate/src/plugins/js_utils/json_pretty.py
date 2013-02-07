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

# This code is based in this code:
# <http://www.muhuk.com/2008/11/extending-kate-with-pate/>
# This file originally was in this repository:
# <https://github.com/goinnn/Kate-plugins/blob/master/kate_plugins/jste_plugins/json_plugins.py>

import kate
import simplejson

from libkatepate import text
from settings_js import KATE_ACTIONS


@kate.action(**KATE_ACTIONS['togglePrettyJsonFormat'])
def togglePrettyJsonFormat():
    """A simple JSON pretty printer. JSON formatter which a good indents"""
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    source = view.selectionText()
    if not source:
        kate.gui.popup('Select a json text', 2,
                       icon='dialog-warning',
                       minTextWidth=200)
    try:
        target = simplejson.dumps(simplejson.loads(source), indent=2)
        view.removeSelectionText()
        text.insertText(target)
    except simplejson.JSONDecodeError:
        kate.gui.popup('This text is not a valid json text', 2,
                       icon='dialog-warning',
                       minTextWidth=200)
