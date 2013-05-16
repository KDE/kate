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
import os

from PyQt4 import uic
from PyQt4.QtGui import QWidget

from libkatepate.errors import needs_packages


from js_snippets import *
from js_autocomplete import *
from json_pretty import *

needs_packages({"pyjslint": "0.3.4"})
from jslint import *

from js_settings import (KATE_CONFIG,
                         _INDENT_JSON_CFG,
                         _ENCODING_JSON_CFG,
                         _JSLINT_CHECK_WHEN_SAVE,
                         _ENABLE_JS_AUTOCOMPLETE,
                         _ENABLE_JQUERY_AUTOCOMPLETE,
                         _ENABLE_TEXT_JQUERY,
                         DEFAULT_TEXT_JQUERY,
                         DEFAULT_INDENT_JSON,
                         DEFAULT_ENCODING_JSON,
                         DEFAULT_CHECK_JSLINT_WHEN_SAVE,
                         DEFAULT_ENABLE_JS_AUTOCOMPLETE,
                         DEFAULT_ENABLE_JQUERY_AUTOCOMPLETE)


_CONFIG_UI = 'js_utils.ui'


class ConfigWidget(QWidget):
    """Configuration widget for this plugin."""

    def __init__(self, parent=None, name=None):
        super(ConfigWidget, self).__init__(parent)
        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), _CONFIG_UI), self)
        self.reset()

    def apply(self):
        kate.configuration[_INDENT_JSON_CFG] = self.indentJSON.value()
        kate.configuration[_ENCODING_JSON_CFG] = self.encodingJSON.text()
        kate.configuration[_ENABLE_TEXT_JQUERY] = self.readyJQuery.toPlainText()
        kate.configuration[_JSLINT_CHECK_WHEN_SAVE] = self.checkJSLintWhenSave.isChecked()
        kate.configuration[_ENABLE_JS_AUTOCOMPLETE] = self.enableJSAutoComplete.isChecked()
        kate.configuration[_ENABLE_JQUERY_AUTOCOMPLETE] = self.enableJQueryAutoComplete.isChecked()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if _INDENT_JSON_CFG in kate.configuration:
            self.indentJSON.setValue(kate.configuration[_INDENT_JSON_CFG])
        if _ENCODING_JSON_CFG in kate.configuration:
            self.encodingJSON.setText(kate.configuration[_ENCODING_JSON_CFG])
        if _ENABLE_TEXT_JQUERY in kate.configuration:
            self.readyJQuery.setPlainText(kate.configuration[_ENABLE_TEXT_JQUERY])
        if _JSLINT_CHECK_WHEN_SAVE in kate.configuration:
            self.checkJSLintWhenSave.setChecked(kate.configuration[_JSLINT_CHECK_WHEN_SAVE])
        if _ENABLE_JS_AUTOCOMPLETE in kate.configuration:
            self.enableJSAutoComplete.setChecked(kate.configuration[_ENABLE_JS_AUTOCOMPLETE])
        if _ENABLE_JQUERY_AUTOCOMPLETE in kate.configuration:
            self.enableJQueryAutoComplete.setChecked(kate.configuration[_ENABLE_JQUERY_AUTOCOMPLETE])

    def defaults(self):
        self.indentJSON.setValue(DEFAULT_INDENT_JSON)
        self.encodingJSON.setText(DEFAULT_ENCODING_JSON)
        self.readyJQuery.setPlainText(DEFAULT_TEXT_JQUERY)
        self.checkJSLintWhenSave.setChecked(DEFAULT_CHECK_JSLINT_WHEN_SAVE)
        self.enableJSAutoComplete.setChecked(DEFAULT_ENABLE_JS_AUTOCOMPLETE)
        self.enableJQueryAutoComplete.setChecked(DEFAULT_ENABLE_JQUERY_AUTOCOMPLETE)


class ConfigPage(kate.Kate.PluginConfigPage, QWidget):
    """Kate configuration page for this plugin."""
    def __init__(self, parent=None, name=None):
        super(ConfigPage, self).__init__(parent, name)
        self.widget = ConfigWidget(parent)
        lo = parent.layout()
        lo.addWidget(self.widget)

    def apply(self):
        self.widget.apply()

    def reset(self):
        self.widget.reset()

    def defaults(self):
        self.widget.defaults()
        self.changed.emit()


@kate.configPage(**KATE_CONFIG)
def configPage(parent=None, name=None):
    return ConfigPage(parent, name)

# kate: space-indent on; indent-width 4;
