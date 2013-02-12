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


DJ_MENU = "Django"

KATE_ACTIONS = {
    'createForm': {'text': 'Create Django Form', 'shortcut': 'Ctrl+Alt+F',
                   'menu': DJ_MENU, 'icon': None},
    'createModel': {'text': 'Create Django Model', 'shortcut': 'Ctrl+Alt+M',
                    'menu': DJ_MENU, 'icon': None},
    'importUrls': {'text': 'Template Django urls', 'shortcut': 'Ctrl+Alt+7',
                   'menu': DJ_MENU, 'icon': None},
    'importViews': {'text': 'Template import views', 'shortcut': 'Ctrl+Alt+I',
                    'menu': DJ_MENU, 'icon': None},
    'createBlock': {'text': 'Template block', 'shortcut': 'Ctrl+Alt+B',
                              'menu': DJ_MENU, 'icon': None},
    'closeTemplateTag': {'text': 'Close Template tag', 'shortcut': 'Ctrl+Alt+C',
                              'menu': DJ_MENU, 'icon': None},
}

TEMPLATE_TAGS_CLOSE = ["autoescape", "block", "comment", "filter", "for",
                       "ifchanged", "ifequal", "if", "spaceless", "with"]

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