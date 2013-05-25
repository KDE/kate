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
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/djte_plugins/block_plugins.py>

import kate
import re

from libkatepate.text import insertText, TEXT_TO_CHANGE
from django_settings import (KATE_ACTIONS,
                             _TEMPLATE_TAGS_CLOSE,
                             DEFAULT_TEMPLATE_TAGS_CLOSE)

str_blank = "(?:\ |\t|\n)*"


@kate.action(**KATE_ACTIONS['closeTemplateTag'])
def closeTemplateTag():
    """Close the last templatetag open"""
    django_utils_conf = kate.configuration.root.get('django_utils', {})
    template_tags_close = django_utils_conf.get(_TEMPLATE_TAGS_CLOSE, DEFAULT_TEMPLATE_TAGS_CLOSE).split(",")
    template_tags_close = [tag.strip() for tag in template_tags_close]
    template_tags = '|'.join(template_tags_close)
    pattern_tag_open = re.compile("(.)*{%%%(espaces)s(%(tags)s)%(espaces)s(.)*%(espaces)s%%}(.)*" % {'espaces': str_blank, 'tags': template_tags})
    pattern_tag_close = re.compile("(.)*{%%%(espaces)send(%(tags)s)%(espaces)s%(espaces)s%%}(.)*" % {'espaces': str_blank, 'tags': template_tags})
    tag_closes = {}
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    currentPosition = view.cursorPosition()
    currentLine = currentPosition.line()
    tag = ''
    while currentLine >= 0:
        text = currentDocument.line(currentLine)
        match_open = pattern_tag_open.match(text)
        if match_open:
            tag_name = match_open.groups()[1]
            if tag_name in tag_closes:
                tag_closes[tag_name] -= 1
                if tag_closes[tag_name] == 0:
                    del tag_closes[tag_name]
            elif not tag_closes:
                tag = match_open.groups()[1]
                break
        else:
            match_close = pattern_tag_close.match(text)
            if match_close:
                tag_name = match_close.groups()[1]
                tag_closes[tag_name] = tag_closes.get(tag_name, 0) + 1
        currentLine = currentLine - 1
    insertText("{%% end%s %%}" % tag)


@kate.action(**KATE_ACTIONS['createBlock'])
def createBlock():
    """Insert the tag block/endblock. The name of the block will be the text selected"""
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    source = view.selectionText()
    try:
        block_type, block_source = source.split("#")
    except ValueError:
        block_type = 'block'
        block_source = source
    view.removeSelectionText()
    insertText("{%% %(block_type)s %(block_source)s %%}%(cursor)s{%% end%(block_type)s %%}" %
               {'block_type': block_type,
                'block_source': block_source,
                'cursor': TEXT_TO_CHANGE})


# kate: space-indent on; indent-width 4;
