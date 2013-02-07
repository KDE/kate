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

JS_MENU = 'JavaScript'


KATE_ACTIONS = {'insertReady': {'text': 'jQuery Ready',
                                'shortcut': 'Ctrl+J',
                                'menu': JS_MENU,
                                'icon': None},
                'checkJslint': {'text': 'JSLint',
                                'shortcut': 'Alt+9',
                                'menu': JS_MENU,
                                'icon': None},
                'togglePrettyJsonFormat': {'text': 'Pretty Json',
                                           'shortcut': 'Ctrl+Alt+J',
                                           'menu': JS_MENU,
                                           'icon': None},
                }

JAVASCRIPT_AUTOCOMPLETE_ENABLED = True
JQUERY_AUTOCOMPLETE_ENABLED = True
CHECK_WHEN_SAVE = True