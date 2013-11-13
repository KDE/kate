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

from __future__ import absolute_import

import kate
import re

from PyKDE4.kdecore import i18nc

from libkatepate.autocomplete import AbstractJSONFileCodeCompletionModel, reset
from .js_settings import SETTING_JS_AUTOCOMPLETE, SETTING_JQUERY_AUTOCOMPLETE


class StaticJSCodeCompletionModel(AbstractJSONFileCodeCompletionModel):
    MIMETYPES = ['application/javascript', 'text/html']
    TITLE_AUTOCOMPLETION = i18nc('@title:menu', 'JavaScript autocomplete')
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
    TITLE_AUTOCOMPLETION = i18nc('@title:menu', 'jQuery autocomplete')
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
        expr = super(StaticJSCodeCompletionModel, self).getLastExpression(line, operators=operators)
        if expr.startswith('$.'):
            expr = expr.replace('$.', 'jQuery.', 1)
        return expr

    def getJSON(self, lastExpression, line):
        if self.object_jquery:
            return self.json['___object']
        return self.json


def init_js_autocomplete(view=None):
    global js_ccm
    if not SETTING_JS_AUTOCOMPLETE.lookup():
        return
    if js_ccm is None:
        js_ccm = StaticJSCodeCompletionModel(kate.application)
        js_ccm.modelReset.connect(reset)
    view = view or kate.activeView()
    cci = view.codeCompletionInterface()
    cci.registerCompletionModel(js_ccm)


def init_jquery_autocomplete(view=None):
    global jquery_ccm
    if not SETTING_JQUERY_AUTOCOMPLETE.lookup():
        return
    if jquery_ccm is None:
        jquery_ccm = StaticJQueryCompletionModel(kate.application)
        jquery_ccm.modelReset.connect(reset)
    view = view or kate.activeView()
    cci = view.codeCompletionInterface()
    cci.registerCompletionModel(jquery_ccm)

js_ccm = None
jquery_ccm = None

# kate: space-indent on; indent-width 4;
