# -*- encoding: utf-8 -*-
# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
# Copyright (C) 2013 Shaheed Haque <srhaque@theiet.org>
# Copyright (C) 2013 Alex Turbov <i.zaufi@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) version 3.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

'''Namespace for Kate application interfaces.

This package provides support for Python plugin developers for the Kate
editor. In addition to the C++ APIs exposed via PyKDE4.ktexteditor and
PyKate4.kate, Python plugins can:

    - Hook into Kate’s menus and shortcuts.
    - Have Kate configuration pages.
    - Use Python pickling to save configuration data, so arbitrary Python
      objects can be part of a plugin’s configuration.
    - Use threaded Python.
    - Use Python2 or Python3 (from Kate 4.11 onwards), depending on how Kate
      was built.
    - Support i18n.
'''


import pate

from .api import *
from .configuration import *
from .decorators import *


@pateEventHandler('_pluginLoaded')
def on_load(plugin):
    if plugin in init.functions:
        # Call registered init functions for the plugin
        init.fire(plugin=plugin)
        del init.functions[plugin]
    return True


@pateEventHandler('_pluginUnloading')
def on_unload(plugin):
    if plugin in unload.functions:
        # Deinitialize plugin
        unload.fire(plugin=plugin)
        del unload.functions[plugin]
    return True


@pateEventHandler('_pateLoaded')
def on_pate_loaded():
    assert(mainWindow is not None)
    qDebug('PATE LOADED')
    return True


@pateEventHandler('_pateUnloading')
def on_pate_unloading():
    qDebug('UNLOADING PATE')
    return True
