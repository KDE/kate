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
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/djte_plugins/text_plugins.py>
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/djte_plugins/class_plugins.py>

import kate

from PyQt4 import QtGui

from libkatepate.text import insertText, TEXT_TO_CHANGE
from django_settings import (KATE_ACTIONS,
                             _TEMPLATE_TEXT_URLS,
                             _TEMPLATE_TEXT_VIEWS,
                             _PATTERN_MODEL_FORM,
                             _PATTERN_MODEL,
                             DEFAULT_TEXT_URLS,
                             DEFAULT_TEXT_VIEWS,
                             DEFAULT_PATTERN_MODEL_FORM,
                             DEFAULT_PATTERN_MODEL)


@kate.action(**KATE_ACTIONS['importUrls'])
def importUrls():
    """Insert the typical code of the urls.py file"""
    currentDocument = kate.activeDocument()
    path = currentDocument.url().directory()
    path_split = path.split('/')
    application = path_split[len(path_split) - 1] or TEXT_TO_CHANGE
    django_utils_conf = kate.configuration.root.get('django_utils', {})
    text_urls = django_utils_conf.get(_TEMPLATE_TEXT_URLS, DEFAULT_TEXT_URLS)
    insertText(text_urls % {'app': application,
                            'change': TEXT_TO_CHANGE})


@kate.action(**KATE_ACTIONS['importViews'])
def importViews():
    """Insert the usual imports of the views.py file"""
    django_utils_conf = kate.configuration.root.get('django_utils', {})
    text_views = django_utils_conf.get(_TEMPLATE_TEXT_VIEWS, DEFAULT_TEXT_VIEWS)
    insertText(text_views)


def create_frame(pattern_str='', title='', name_field=''):
    currentDocument = kate.activeDocument()
    view = kate.activeView()
    class_name, ok = QtGui.QInputDialog.getText(view, title, name_field)
    if ok:
        class_model = class_name.replace('Form', '')
        text = pattern_str % {'class_name': class_name,
                              'class_model': class_model}
        currentDocument.insertText(view.cursorPosition(), text)


@kate.action(**KATE_ACTIONS['createForm'])
def createForm():
    """Create a form class from its name"""
    django_utils_conf = kate.configuration.root.get('django_utils', {})
    pattern_model_form = django_utils_conf.get(_PATTERN_MODEL_FORM, DEFAULT_PATTERN_MODEL_FORM)
    create_frame(pattern_str=pattern_model_form, title='Create Form',
                 name_field='Name Form')


@kate.action(**KATE_ACTIONS['createModel'])
def createModel():
    """Create a model class from its name"""
    django_utils_conf = kate.configuration.root.get('django_utils', {})
    pattern_model = django_utils_conf.get(_PATTERN_MODEL, DEFAULT_PATTERN_MODEL)
    create_frame(pattern_str=pattern_model, title='Create Model',
                 name_field='Name Model')


# kate: space-indent on; indent-width 4;
