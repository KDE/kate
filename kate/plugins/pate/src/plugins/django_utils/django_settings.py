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
    'importViews': {'text': 'Template import views', 'shortcut': 'Ctrl+Alt+v',
                    'menu': DJ_MENU, 'icon': None},
    'createBlock': {'text': 'Template block', 'shortcut': 'Ctrl+Alt+B',
                              'menu': DJ_MENU, 'icon': None},
    'closeTemplateTag': {'text': 'Close Template tag', 'shortcut': 'Ctrl+Alt+C',
                              'menu': DJ_MENU, 'icon': None},
}

TEMPLATE_TAGS_CLOSE = ["autoescape", "block", "comment", "filter", "for",
                       "ifchanged", "ifequal", "if", "spaceless", "with"]
