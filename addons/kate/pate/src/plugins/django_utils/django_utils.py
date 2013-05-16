# -*- coding: utf-8 -*-
'''Django utilities: Snippets, and template utilities'''

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
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/djte_plugins/>

import kate
import os

from PyQt4 import uic
from PyQt4.QtGui import QWidget

from django_settings import (KATE_CONFIG,
                             _TEMPLATE_TAGS_CLOSE,
                             _TEMPLATE_TEXT_URLS,
                             _TEMPLATE_TEXT_VIEWS,
                             _PATTERN_MODEL_FORM,
                             _PATTERN_MODEL,
                             DEFAULT_TEMPLATE_TAGS_CLOSE,
                             DEFAULT_TEXT_URLS,
                             DEFAULT_TEXT_VIEWS,
                             DEFAULT_PATTERN_MODEL_FORM,
                             DEFAULT_PATTERN_MODEL)
from django_snippets import *
from django_template import *


_CONFIG_UI = 'django_utils.ui'


class ConfigWidget(QWidget):
    """Configuration widget for this plugin."""

    def __init__(self, parent=None, name=None):
        super(ConfigWidget, self).__init__(parent)
        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), _CONFIG_UI), self)
        self.reset()

    def apply(self):
        kate.configuration[_TEMPLATE_TAGS_CLOSE] = self.templateTagsClose.text()
        kate.configuration[_TEMPLATE_TEXT_URLS] = self.templateTextURLs.toPlainText()
        kate.configuration[_TEMPLATE_TEXT_VIEWS] = self.templateTextViews.toPlainText()
        kate.configuration[_PATTERN_MODEL_FORM] = self.patternModelForm.toPlainText()
        kate.configuration[_PATTERN_MODEL] = self.patternModel.toPlainText()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if _TEMPLATE_TAGS_CLOSE in kate.configuration:
            self.templateTagsClose.setText(kate.configuration[_TEMPLATE_TAGS_CLOSE])
        if _TEMPLATE_TEXT_URLS in kate.configuration:
            self.templateTextURLs.setPlainText(kate.configuration[_TEMPLATE_TEXT_URLS])
        if _TEMPLATE_TEXT_VIEWS in kate.configuration:
            self.templateTextViews.setPlainText(kate.configuration[_TEMPLATE_TEXT_VIEWS])
        if _PATTERN_MODEL_FORM in kate.configuration:
            self.patternModelForm.setPlainText(kate.configuration[_PATTERN_MODEL_FORM])
        if _PATTERN_MODEL in kate.configuration:
            self.patternModel.setPlainText(kate.configuration[_PATTERN_MODEL])

    def defaults(self):
        self.templateTagsClose.setText(DEFAULT_TEMPLATE_TAGS_CLOSE)
        self.templateTextURLs.setPlainText(DEFAULT_TEXT_URLS)
        self.templateTextViews.setPlainText(DEFAULT_TEXT_VIEWS)
        self.patternModelForm.setPlainText(DEFAULT_PATTERN_MODEL_FORM)
        self.patternModel.setPlainText(DEFAULT_PATTERN_MODEL)


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
