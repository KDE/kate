# -*- coding: utf-8 -*-
'''Python Utilities: Parse Checker, PEP8 Checker, Pyflakes Checker, Snippets'''

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
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/>
# The original author of the pep8 and pyflakes checker is Alejandro Blanco <alejandro.b.e@gmail.com>

import os

from PyQt4 import uic
from PyQt4.QtGui import QWidget

from libkatepate.compat import text_type

from python_snippets import *
from python_checkers.parse_checker import *

from python_settings import (KATE_CONFIG,
                             _PEP8_CHECK_WHEN_SAVE,
                             _IGNORE_PEP8_ERRORS,
                             _PYFLAKES_CHECK_WHEN_SAVE,
                             _PARSECODE_CHECK_WHEN_SAVE,
                             _IPDB_SNIPPET,
                             DEFAULT_CHECK_PEP8_WHEN_SAVE,
                             DEFAULT_IGNORE_PEP8_ERRORS,
                             DEFAULT_CHECK_PYFLAKES_WHEN_SAVE,
                             DEFAULT_PARSECODE_CHECK_WHEN_SAVE,
                             DEFAULT_IPDB_SNIPPET)


_CONFIG_UI = 'python_utils.ui'


class ConfigWidget(QWidget):
    """Configuration widget for this plugin."""

    def __init__(self, parent=None, name=None):
        super(ConfigWidget, self).__init__(parent)
        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), _CONFIG_UI), self)
        self.reset()

    def apply(self):
        kate.configuration[_PEP8_CHECK_WHEN_SAVE] = self.checkPEP8WhenSave.isChecked()
        kate.configuration[_IGNORE_PEP8_ERRORS] = self.ignorePEP8Errors.text()
        kate.configuration[_PYFLAKES_CHECK_WHEN_SAVE] = self.checkPyFlakesWhenSave.isChecked()
        kate.configuration[_PARSECODE_CHECK_WHEN_SAVE] = self.checkParseCode.isChecked()
        kate.configuration[_IPDB_SNIPPET] = self.ipdbSnippet.text()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if _PEP8_CHECK_WHEN_SAVE in kate.configuration:
            self.checkPEP8WhenSave.setChecked(kate.configuration[_PEP8_CHECK_WHEN_SAVE])
        if _IGNORE_PEP8_ERRORS in kate.configuration:
            self.ignorePEP8Errors.setText(kate.configuration[_IGNORE_PEP8_ERRORS])
        if _PYFLAKES_CHECK_WHEN_SAVE in kate.configuration:
            self.checkPyFlakesWhenSave.setChecked(kate.configuration[_PYFLAKES_CHECK_WHEN_SAVE])
        if _PARSECODE_CHECK_WHEN_SAVE in kate.configuration:
            self.checkParseCode.setChecked(kate.configuration[_PARSECODE_CHECK_WHEN_SAVE])
        if _IPDB_SNIPPET in kate.configuration:
            self.ipdbSnippet.setText(kate.configuration[_IPDB_SNIPPET])

    def defaults(self):
        self.checkPEP8WhenSave.setChecked(DEFAULT_CHECK_PEP8_WHEN_SAVE)
        self.ignorePEP8Errors.setText(DEFAULT_IGNORE_PEP8_ERRORS)
        self.checkPyFlakesWhenSave.setChecked(DEFAULT_CHECK_PYFLAKES_WHEN_SAVE)
        self.checkParseCode.setChecked(DEFAULT_PARSECODE_CHECK_WHEN_SAVE)
        self.ipdbSnippet.setText(DEFAULT_IPDB_SNIPPET)


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
