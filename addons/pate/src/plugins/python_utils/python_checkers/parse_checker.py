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
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/check_plugins/parse_plugins.py>

import kate

try:
    from compiler import parse
except ImportError:
    from ast import parse

from PyKDE4.kdecore import i18n
from libkatepate.errors import showOk, showErrors

from python_checkers.all_checker import checkAll
from python_checkers.utils import canCheckDocument
from python_settings import KATE_ACTIONS


@kate.action(**KATE_ACTIONS['parseCode'])
def parseCode(doc=None, refresh=True):
    """Check the syntax errors of the document"""
    if not canCheckDocument(doc):
        return
    if refresh:
        checkAll.f(doc, ['parseCode'], exclude_all=not doc)
    move_cursor = not doc
    doc = doc or kate.activeDocument()
    text = doc.text()
    text = text.encode('utf-8', 'ignore')
    mark_key = '%s-parse-python' % doc.url().path()
    try:
        parse(text)
        showOk(i18n('Parse code Ok'))
    except SyntaxError as e:
        error = {}
        error['text'] = e.text
        error['line'] = e.lineno
        showErrors(i18n('Parse code Errors:'), [error], mark_key, doc,
                   move_cursor=move_cursor)

# kate: space-indent on; indent-width 4;
