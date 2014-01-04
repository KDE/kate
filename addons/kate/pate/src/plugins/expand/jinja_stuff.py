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

'''Jinja renderer and related stuff'''

import functools
import os
import re

import jinja2

import kate
import kate.ui

from PyKDE4.kdecore import i18nc

from .settings import *


_JINJA_TEMPLATES_BASE_DIR = os.path.join(EXPANDS_BASE_DIR, 'templates')


def jinja_environment_configurator(func):
    '''
        ATTENTION Be aware that functions marked w/ jinja_environment_configurator
        decorator should have two leading underscores in their name!
        Otherwise, they will be available as ordinal expand functions, which is
        definitely UB. This made to keep `expand` code simple...
    '''
    if not hasattr(jinja_environment_configurator, 'registered_configurators'):
        setattr(jinja_environment_configurator, 'registered_configurators', dict())
    mimeType = kate.activeDocument().mimeType()
    jinja_environment_configurator.registered_configurators[mimeType] = func
    kate.qDebug('Set jinja2 environment configurator for {} to {}'.format(mimeType, func.__name__))
    return func


def _makeEditableField(param, **kw):
    act = '@' if 'active' in kw else ''
    if 'name' in kw:
        name = kw['name']
    else:
        name = param.strip()
    return '${{{}{}:{}}}'.format(name, act, param)


def _is_bool(s):
    return isinstance(s,bool)


def _is_int(s):
    return isinstance(s,int)


@functools.lru_cache()
def _getJinjaEnvironment(baseDir):
    kate.qDebug('Make a templates loader for a base dir: {}'.format(baseDir))
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(baseDir))

    env.filters['editable'] = _makeEditableField
    env.tests['boolean'] = _is_bool
    env.tests['integer'] = _is_int

    if hasattr(jinja_environment_configurator, 'registered_configurators'):
        mimeType = kate.activeDocument().mimeType()
        configurator = jinja_environment_configurator.registered_configurators[mimeType]
        kate.qDebug('Setup jinja2 environment for {} [{}]'.format(mimeType, configurator.__name__))
        env = configurator(env)

    return env


def render_jinja_template(template, data):
    assert(isinstance(data, dict))
    assert(isinstance(template, str))

    # Add some predefined variables
    # - `nl` == new line character
    if 'nl' not in data:
        data['nl'] = '\n'
    # - `tab` == one TAB character
    if 'tab' not in data:
        data['tab'] = '\t'
    # - `space` == one space character
    if 'space' not in data:
        data['space'] = ' '

    result = None
    # Ok, going to render some jinja2 template...
    filename = kate.findApplicationResource('{}/{}'.format(_JINJA_TEMPLATES_BASE_DIR, template))
    if not filename:
        kate.ui.popup(
            i18nc('@title:window', 'Error')
            , i18nc('@info:tooltip', 'Template file not found <filename>%1</filename>', template)
            , 'dialog-error'
            )
        return result

    kate.qDebug('found abs template: {}'.format(filename))

    # Get a corresponding environment for jinja!
    base_dir_pos = filename.find(_JINJA_TEMPLATES_BASE_DIR)
    assert(base_dir_pos != -1)
    basedir = filename[:base_dir_pos + len(_JINJA_TEMPLATES_BASE_DIR)]
    filename = filename[base_dir_pos + len(_JINJA_TEMPLATES_BASE_DIR) + 1:]
    env = _getJinjaEnvironment(basedir)
    kate.qDebug('basedir={}, template_rel={}'.format(basedir, filename))
    try:
        tpl = env.get_template(filename)
        kate.qDebug('data dict={}'.format(data))
        result = tpl.render(data)
    except jinja2.TemplateError as e:
        kate.ui.popup(
            i18nc('@title:window', 'Error')
            , i18nc(
                '@info:tooltip'
                , 'Template file error [<filename>%1</filename>]: <status>%2</status>'
                , template
                , e.message
                )
            , 'dialog-error'
            )
    return result
