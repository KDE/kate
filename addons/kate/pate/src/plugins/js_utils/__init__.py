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

from __future__ import absolute_import

import kate

from libkatepate import text

from PyKDE4.kdecore import i18nc

from libkatepate.decorators import *

from .js_settings import SETTING_JQUERY_READY
from .js_config_page import ConfigPage
from .js_lint import lint_js, init_js_lint
from .js_autocomplete import init_js_autocomplete, init_jquery_autocomplete
from .json_pretty import prettify_JSON


@kate.configPage(
    i18nc('@item:inmenu', 'JavaScript Utilities')
  , i18nc('@title:group', 'JavaScript Utilities Options')
  , 'application-x-javascript'
  )
def configPage(parent=None, name=None):
    return ConfigPage(parent, name)


@kate.action
def lint_js_action():
    """Lints the active document"""
    lint_js(kate.activeDocument(), True)


@kate.action
def insert_jquery_ready_action():
    """Snippet with the ready code of the jQuery"""
    text.insertText(SETTING_JQUERY_READY.lookup(), start_in_current_column=True)


@kate.action
@check_constraints
@has_selection(True)
def prettify_JSON_action():
    prettify_JSON(kate.activeView())


@kate.viewCreated
def init(view=None):
    init_js_lint(view)
    init_js_autocomplete(view)
    init_jquery_autocomplete(view)


# kate: space-indent on; indent-width 4;
