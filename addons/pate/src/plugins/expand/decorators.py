# -*- coding: utf-8 -*-
#
# This file is part of Kate's Expand plugin.
#
# Copyright (C) 2013 Alex Turbov <i.zaufi@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

'''Decorators for expand functions'''

import inspect
import re

import kate

from PyKDE4.ktexteditor import KTextEditor

from .udf import getExpansionsFor

def jinja(template):
    ''' Decorator to tell to the expand engine that decorated function
        wants to use jinja2 template to render, instead of 'legacy'
        (document.insertText()) way...
    '''
    def _decorator(func):
        func.template = template
        return func
    return _decorator


def postprocess(func):
    ''' Decorator to tell to the expand engine that decorated function
        wants to use TemplateInterface2 to insert text, so user may
        tune a rendered template...
    '''
    func.use_template_iface = True
    return func


def description(text):
    def _decorator(func):
        setattr(func, '__description__', text)
        return func
    return _decorator


def details(text):
    def _decorator(func):
        setattr(func, '__details__', text)
        return func
    return _decorator


def dynamic(regex):
    def _decorator(func):
        if not hasattr(dynamic, 'registered_handlers'):
            setattr(dynamic, 'registered_handlers', dict())
        mimeType = kate.activeDocument().mimeType()
        if mimeType not in dynamic.registered_handlers:
            dynamic.registered_handlers[mimeType] = list()
        dynamic.registered_handlers[mimeType].append(func)
        setattr(func, 'match_regex', regex)
        kate.qDebug('Set KS expander for {} to {}'.format(mimeType, func.__name__))
        return func
    return _decorator
