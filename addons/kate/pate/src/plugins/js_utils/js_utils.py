# -*- coding: utf-8 -*-
'''JavaScript utilities: Autocomplete, Autocomplete jQuery, Checker JSLint, Pretty JSON, Snippets'''

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
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/jste_plugins/>
# The original author of the jslint checker is Alejandro Blanco <alejandro.b.e@gmail.com>

import kate

from libkatepate import text

from PyKDE4.kdecore import i18nc

from js_settings import SETTING_JQUERY_READY
from js_config_page import ConfigPage
from js_lint import lint_js, init_js_lint
from js_autocomplete import init_js_autocomplete, init_jquery_autocomplete
from json_pretty import prettify_JSON, PRETTIFY_JSON_SHORTCUT


# use for full and short name as short gets used in the menu
_CFG_PAGE_NAME = i18nc('@title:group', 'JavaScript Utilites')


@kate.configPage(_CFG_PAGE_NAME, _CFG_PAGE_NAME, 'application-x-javascript')
def configPage(parent=None, name=None):
    return ConfigPage(parent, name)


_JS_MENU = 'JavaScript'


@kate.action(i18nc('@action:inmenu', 'Lint JavaScript'), None, 'Alt+9', _JS_MENU)
def lint_js_action():
    """Lints the active document"""
    lint_js(kate.activeDocument(), True)


@kate.action(i18nc('@action:inmenu', 'jQuery Ready'), None, 'Ctrl+Shift+J', _JS_MENU)
def insert_jquery_ready_action():
    """Snippet with the ready code of the jQuery"""
    text.insertText(SETTING_JQUERY_READY.lookup(), start_in_current_column=True)


@kate.action(i18nc('@action:inmenu', 'Prettify JSON'), None, PRETTIFY_JSON_SHORTCUT, _JS_MENU)
def prettify_JSON_action():
    prettify_JSON(kate.activeView())


@kate.init
@kate.viewCreated
def init(view=None):
    init_js_lint(view)
    init_js_autocomplete(view)
    init_jquery_autocomplete(view)


# kate: space-indent on; indent-width 4;
