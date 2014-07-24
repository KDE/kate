# -*- coding: utf-8 -*-
'''A simple XML pretty printer. It formats XML with good indentiation.'''
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
# <https://github.com/goinnn/Kate-plugins/blob/master/kate_plugins/xhtml_plugins/xml_plugins.py>

import codecs
import kate
import os
import re

from xml.dom import minidom
from xml.parsers.expat import ExpatError

from PyKDE4.kdecore import i18n
from PyQt4 import uic
from PyQt4.QtGui import QWidget

from kate import qDebug

from libkatepate import selection
from libkatepate.errors import showError
from libkatepate.decorators import *


KATE_CONFIG = {'name': 'XML Pretty',
               'fullName': 'XML Pretty',
               'icon': 'text-xml'}

_CONFIG_UI = 'xml_pretty.ui'
_INDENT_CFG = 'XMLPretty:indentXXX'
_NEWL_CFG = 'XMLPretty:newlXXX'

# TODO Add more appropriate XML(-like) types (XSLT,HTML,?)
_ALLOWED_MIME_TYPES = ['application/xml']

DEFAULT_INDENT = '\\t'
DEFAULT_NEWL = '\\n'

encoding_pattern = re.compile("<\?xml[\w =\"\.]*encoding=[\"']([\w-]+)[\"'][\w =\"\.]*\?>")


@kate.action
@check_constraints
@has_selection(True)
@selection_mode(selection.NORMAL)
#@restrict_doc_type('XML', 'xslt')
def prettyXMLFormat():
    """Pretty format of a XML code"""
    # TODO Use decorators to apply constraints
    document = kate.activeDocument()
    view = document.activeView()
    try:
        encoding = 'utf-8'
        source = view.selectionText()
        m = encoding_pattern.match(source)
        if m:
            encoding = m.groups()[0]
        target = minidom.parseString(source.encode(encoding))
        unicode_escape = codecs.getdecoder('unicode_escape')
        indent = unicode_escape(kate.configuration.get(_INDENT_CFG, DEFAULT_INDENT))[0]
        newl = unicode_escape(kate.configuration.get(_NEWL_CFG, DEFAULT_NEWL))[0]
        xml_pretty = target.toprettyxml(indent=indent, newl=newl, encoding=encoding).decode(encoding)
        xml_pretty = newl.join([line for line in xml_pretty.split(newl) if line.replace(' ', '').replace(indent, '')])
        document.replaceText(view.selectionRange(), xml_pretty)
    except (ExpatError, LookupError) as e:
        showError(i18nc('@info:tooltip', 'The selected text is not valid XML: %1', str(e)))


class ConfigWidget(QWidget):
    """Configuration widget for this plugin."""

    def __init__(self, parent=None, name=None):
        super(ConfigWidget, self).__init__(parent)

        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), _CONFIG_UI), self)
        self.reset()

    def apply(self):
        kate.configuration[_NEWL_CFG] = self.newl.text()
        kate.configuration[_INDENT_CFG] = self.indent.text()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if _INDENT_CFG in kate.configuration:
            self.indent.setText(kate.configuration[_INDENT_CFG])
        if _NEWL_CFG in kate.configuration:
            self.newl.setText(kate.configuration[_NEWL_CFG])

    def defaults(self):
        self.indent.setText(DEFAULT_INDENT)
        self.newl.setText(DEFAULT_NEWL)


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

