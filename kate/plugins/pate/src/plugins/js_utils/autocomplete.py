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
# <https://github.com/goinnn/Kate-plugins/blob/master/kate_plugins/jste_plugins/autocomplete.py>

import kate
import re

from libkatepate.autocomplete import AbstractJSONFileCodeCompletionModel, reset
from settings import (JAVASCRIPT_AUTOCOMPLETE_ENABLED,
                      JQUERY_AUTOCOMPLETE_ENABLED)


class StaticJSCodeCompletionModel(AbstractJSONFileCodeCompletionModel):

    MIMETYPES = ['application/javascript', 'text/html']
    TITLE_AUTOCOMPLETION = "JS Auto Complete"
    # generated with autocomplete_js.gen
    FILE_PATH = 'autocomplete_js.json'
    OPERATORS = ["=", " ", "[", "]", "(", ")", "{", "}", ":", ">", "<",
                 "+", "-", "*", "/", "%", " && ", " || ", ","]

    def __init__(self, *args, **kwargs):
        super(StaticJSCodeCompletionModel, self).__init__(*args, **kwargs)
        self.json['window'] = {'category': 'module', 'children': {}}

    def getLastExpression(self, line, operators=None):
        expr = super(StaticJSCodeCompletionModel, self).getLastExpression(line,
                                                           operators=operators)
        if expr.startswith("window."):
            expr = expr.replace('window.', '', 1)
        return expr


class StaticJQueryCompletionModel(StaticJSCodeCompletionModel):
    TITLE_AUTOCOMPLETION = "jQuery Auto Complete"
    FILE_PATH = 'autocomplete_jquery.json'
    # generated with autocomplete_jquery.gen

    def __init__(self, *args, **kwargs):
        super(StaticJSCodeCompletionModel, self).__init__(*args, **kwargs)
        self.expr = re.compile('(?:.)*[$|jQuery]\(["|\']?(?P<dom_id>[\.\-_\w]+)["|\']?\)\.(?P<query>[\.\-_\w]+)*$')
        self.object_jquery = False

    @classmethod
    def createItemAutoComplete(cls, text, *args, **kwargs):
        if text == '___object':
            return None
        return super(StaticJQueryCompletionModel, cls).createItemAutoComplete(text, *args, **kwargs)

    def getLastExpression(self, line, operators=None):
        m = self.expr.match(line)
        self.object_jquery = m
        if m:
            return m.groups()[1] or ''
        expr = super(StaticJSCodeCompletionModel, self).getLastExpression(line,
                                                           operators=operators)
        if expr.startswith("$."):
            expr = expr.replace('$.', 'jQuery.', 1)
        return expr

    def getJSON(self, lastExpression, line):
        if self.object_jquery:
            return self.json['___object']
        return self.json


@kate.init
@kate.viewCreated
def createSignalAutocompleteJS(view=None, *args, **kwargs):
    if not JAVASCRIPT_AUTOCOMPLETE_ENABLED:
        return
    view = view or kate.activeView()
    cci = view.codeCompletionInterface()
    cci.registerCompletionModel(jscodecompletationmodel)


@kate.init
@kate.viewCreated
def createSignalAutocompletejQuery(view=None, *args, **kwargs):
    if not JQUERY_AUTOCOMPLETE_ENABLED:
        return
    view = view or kate.activeView()
    cci = view.codeCompletionInterface()
    cci.registerCompletionModel(jquerycodecompletationmodel)


if JAVASCRIPT_AUTOCOMPLETE_ENABLED or JQUERY_AUTOCOMPLETE_ENABLED:
    if JAVASCRIPT_AUTOCOMPLETE_ENABLED:
        jscodecompletationmodel = StaticJSCodeCompletionModel(kate.application)
        jscodecompletationmodel.modelReset.connect(reset)

    if JQUERY_AUTOCOMPLETE_ENABLED:
        jquerycodecompletationmodel = StaticJQueryCompletionModel(kate.application)
        jquerycodecompletationmodel.modelReset.connect(reset)
