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

import kate
import os
import sys

from copy import copy


def get_project_plugin():
    mainWindow = kate.mainInterfaceWindow()
    projectPluginView = mainWindow.pluginView("kateprojectplugin")
    return projectPluginView


def is_version_compatible(version):
    if not version:
        return True
    version_info = version.split(".")
    if len(version_info) == 0:
        return False
    elif len(version_info) == 1:
        return int(version_info[0]) == sys.version_info.major
    elif len(version_info) == 2:
        return int(version_info[0]) == sys.version_info.major and \
            int(version_info[1]) == sys.version_info.minor
    else:
        return int(version_info[0]) == sys.version_info.major and \
            int(version_info[1]) == sys.version_info.minor and \
            int(version_info[2]) == sys.version_info.micro


def add_extra_path(extra_path):
    if not getattr(sys, "original_path", None):
        sys.original_path = copy(sys.path)
        sys.original_modules = sys.modules.keys()
    else:
        i = len(sys.path) - 1
        while i >= 0:
            path = sys.path[i]
            if not path in sys.original_path:
                del sys.path[i]
            i = i - 1
        for module in sys.modules.keys():
            if not module in sys.original_modules:
                del sys.modules[module]
    extra_path.reverse()
    for path in extra_path:
        if not path in sys.path:
            sys.path.insert(0, path)
    sys.path_importer_cache = {}


def add_environs(environs):
    for key, value in environs.items():
        if key in os.environ:
            os.environ.pop(key)
        os.environ.setdefault(key, value)
