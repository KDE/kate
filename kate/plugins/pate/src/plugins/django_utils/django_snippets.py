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
from django_settings import KATE_ACTIONS

TEXT_URLS = """from django.conf.urls.defaults import patterns, url


urlpatterns = patterns('%(app)s.views',
    url(r'^$', '%(change)s', name='%(change)s'),
)
"""

TEXT_VIEWS = """from django.core.urlresolvers import reverse
from django.http import HttpResponseRedirect, HttpResponse
from django.shortcuts import render_to_response, get_object_or_404
from django.template import RequestContext
"""

PATTERN_MODEL_FORM = """

class %(class_name)s(forms.ModelForm):

    class Meta:
        model = %(class_model)s

    def __init__(self, *args, **kwargs):
        super(%(class_name)s, self).__init__(*args, **kwargs)

    def clean(self):
        return super(%(class_name)s, self).clean()

    def save(self, commit=True):
        return super(%(class_name)s, self).save(commit)

"""

PATTERN_MODEL = """

class %(class_name)s(models.Model):

    class Meta:
        verbose_name = _('%(class_name)s')
        verbose_name_plural = _('%(class_name)ss')

    @permalink
    def get_absolute_url(self):
        pass

    def __unicode__(self):
        pass

"""


@kate.action(**KATE_ACTIONS['importUrls'])
def importUrls():
    currentDocument = kate.activeDocument()
    path = unicode(currentDocument.url().directory())
    path_split = path.split('/')
    application = path_split[len(path_split) - 1] or TEXT_TO_CHANGE
    insertText(TEXT_URLS % {'app': application,
                            'change': TEXT_TO_CHANGE})


@kate.action(**KATE_ACTIONS['importViews'])
def importViews():
    insertText(TEXT_VIEWS)


def create_frame(pattern_str='', title='', name_field=''):
    currentDocument = kate.activeDocument()
    view = kate.activeView()
    class_name, ok = QtGui.QInputDialog.getText(view, title, name_field)
    if ok:
        class_name = unicode(class_name)
        class_model = class_name.replace('Form', '')
        text = pattern_str % {'class_name': class_name,
                              'class_model': class_model}
        currentDocument.insertText(view.cursorPosition(), text)


@kate.action(**KATE_ACTIONS['createForm'])
def createForm():
    create_frame(pattern_str=PATTERN_MODEL_FORM, title='Create Form',
                 name_field='Name Form')


@kate.action(**KATE_ACTIONS['createModel'])
def createModel():
    create_frame(pattern_str=PATTERN_MODEL, title='Create Model',
                 name_field='Name Model')
