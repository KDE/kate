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

from PyKDE4.kdecore import i18n

from libkatepate.autocomplete import AbstractJSONFileCodeCompletionModel, reset
from js_settings import (DEFAULT_ENABLE_JS_AUTOCOMPLETE,
                         DEFAULT_ENABLE_JQUERY_AUTOCOMPLETE,
                         _ENABLE_JS_AUTOCOMPLETE,
                         _ENABLE_JQUERY_AUTOCOMPLETE)


class StaticJSCodeCompletionModel(AbstractJSONFileCodeCompletionModel):

    MIMETYPES = ['application/javascript', 'text/html']
    TITLE_AUTOCOMPLETION = i18n("JS autocomplete")
    # generated with autocomplete_js.gen
    FILE_PATH = 'js_autocomplete.json'
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
    TITLE_AUTOCOMPLETION = i18n("jQuery autocomplete")
    FILE_PATH = 'jquery_autocomplete.json'
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
    global ENABLE_JS_AUTOCOMPLETE
    if not ENABLE_JS_AUTOCOMPLETE:
        return
    view = view or kate.activeView()
    cci = view.codeCompletionInterface()
    cci.registerCompletionModel(jscodecompletationmodel)


@kate.init
@kate.viewCreated
def createSignalAutocompletejQuery(view=None, *args, **kwargs):
    global ENABLE_JQUERY_AUTOCOMPLETE
    if not ENABLE_JQUERY_AUTOCOMPLETE:
        return
    view = view or kate.activeView()
    cci = view.codeCompletionInterface()
    cci.registerCompletionModel(jquerycodecompletationmodel)


js_utils_conf = kate.configuration.root.get('js_utils', {})
ENABLE_JS_AUTOCOMPLETE = js_utils_conf.get(_ENABLE_JS_AUTOCOMPLETE,
                                           DEFAULT_ENABLE_JS_AUTOCOMPLETE)
ENABLE_JQUERY_AUTOCOMPLETE = js_utils_conf.get(_ENABLE_JQUERY_AUTOCOMPLETE,
                                               DEFAULT_ENABLE_JQUERY_AUTOCOMPLETE)

if ENABLE_JS_AUTOCOMPLETE:
    jscodecompletationmodel = StaticJSCodeCompletionModel(kate.application)
    jscodecompletationmodel.modelReset.connect(reset)

if ENABLE_JQUERY_AUTOCOMPLETE:
    jquerycodecompletationmodel = StaticJQueryCompletionModel(kate.application)
    jquerycodecompletationmodel.modelReset.connect(reset)

# kate: space-indent on; indent-width 4;
