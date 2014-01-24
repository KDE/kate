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
import os.path as p

from PyQt4 import uic
from PyQt4.QtGui import QWidget

from .js_settings import (BoundSetting,
                          SETTING_INDENT_JSON,
                          SETTING_ENCODING_JSON,
                          SETTING_LINTER,
                          SETTING_LINT_ON_SAVE,
                          SETTING_JS_AUTOCOMPLETE,
                          SETTING_JQUERY_AUTOCOMPLETE,
                          SETTING_JQUERY_READY)

_CONFIG_UI = 'js_utils.ui'


class ConfigWidget(QWidget):
    """Configuration widget for this plugin."""
    def __init__(self, parent=None, name=None):
        super(ConfigWidget, self).__init__(parent)
        # Set up the user interface from Designer.
        uic.loadUi(p.join(p.dirname(__file__), _CONFIG_UI), self)

        self.settings = [
            BoundSetting(SETTING_INDENT_JSON,         self.jsonIndent),
            BoundSetting(SETTING_ENCODING_JSON,       self.jsonEncoding),
            BoundSetting(SETTING_LINTER,              self.linter),
            BoundSetting(SETTING_LINT_ON_SAVE,        self.lintOnSave),
            BoundSetting(SETTING_JS_AUTOCOMPLETE,     self.jsAutocompletion),
            BoundSetting(SETTING_JQUERY_AUTOCOMPLETE, self.jQueryAutocompletion),
            BoundSetting(SETTING_JQUERY_READY,        self.jQueryReady),
        ]

    def apply(self):
        for setting in self.settings:
            setting.save()
        kate.configuration.save()

    def reset_widget(self):
        for setting in self.settings:
            setting.setter(setting.lookup())

    def reset(self):
        """resets to defaults and saves that"""
        self.defaults()
        for setting in self.settings:
            if setting.key in kate.configuration:
                setting.save()
        kate.configuration.save()

    def defaults(self):
        for setting in self.settings:
            setting.reset()


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


# kate: space-indent on; indent-width 4;
