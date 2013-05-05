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

# This code originally was in this repository:
# <https://github.com/goinnn/Kate-plugins/blob/master/kate_plugins/autopate.py>
# Thanks to Jeroen van Veen <j.veenvan@gmail.com> for the help

import inspect
import os

from PyKDE4.kdecore import i18n
from PyKDE4.kdeui import KIcon
from PyKDE4.ktexteditor import KTextEditor

from PyQt4.QtCore import QModelIndex, QSize, Qt

try:
    import simplejson as json
except ImportError:
    import json


CCM = KTextEditor.CodeCompletionModel


class AbstractCodeCompletionModel(CCM):

    class GroupPosition:
        BEST_MATCHES = 1
        LOCAL_SCOPE = 100
        PUBLIC = 200
        PROTECTED = 300
        PRIVATE = 400
        NAMESPACE = 500
        GLOBAL = 600

    TITLE_AUTOCOMPLETION = i18n('Autopate')
    MIMETYPES = []
    OPERATORS = []
    SEPARATOR = '.'
    MAX_DESCRIPTION = 80
    GROUP_POSITION = GroupPosition.BEST_MATCHES

    # NOTE Allow to derived classes to override categories set,
    # as well as its mapping to an icon
    CATEGORY_2_ICON = {
        'package': 'code-block',
        'module': 'code-context',
        'unknown': None,
        'constant': 'code-variable',
        'class': 'code-class',
        'function': 'code-function',
        'pointer': 'unknown'
    }

    def __init__(self, model, resultList=None):
        self.model = model
        super(AbstractCodeCompletionModel, self).__init__(model)
        self.resultList = []

    @classmethod
    def createItemAutoComplete(cls, text, category='unknown', args=None, description=None):
        # TODO Add `prefix` parameter
        if description and 0 < cls.MAX_DESCRIPTION and len(description) > cls.MAX_DESCRIPTION:
            description = description.strip()
            description = description[:cls.MAX_DESCRIPTION] + '...'
        return {
            'text': text,
            'icon': cls.CATEGORY_2_ICON[category] if category in cls.CATEGORY_2_ICON else None,
            'category': category,
            'args': args or '',
            'type': category,
            'description': description or ''
        }

    def completionInvoked(self, view, word, invocationType):
        line_start = word.start().line()
        line_end = word.end().line()
        column_end = word.end().column()
        self.resultList = []
        self.invocationType = invocationType
        if line_start != line_end:
            return None
        mimetype = view.document().mimeType()
        if not mimetype in self.MIMETYPES:
            return None
        doc = view.document()
        line = doc.line(line_start)
        if not line:
            return line
        return self.parseLine(line, column_end)

    def data(self, index, role):
        # Check if 'gorup' node requested
        if not index.parent().isValid():
            # Yep, return title and some other gorup props
            if role == CCM.InheritanceDepth:
                return self.GROUP_POSITION
            # ATTENTION TODO NOTE
            # Due this BUG (https://bugs.kde.org/show_bug.cgi?id=247896)
            # we can't use CCM.GroupRole, so hardcoded value 47 is here!
            if role == 47:
                return Qt.DisplayRole
            if role == Qt.DisplayRole:
                return self.TITLE_AUTOCOMPLETION
            # Return 'invalid' for other roles
            return None

        # Leaf item props are requested
        item = self.resultList[index.row()]
        if index.column() == CCM.Name:
            if role == Qt.DisplayRole:
                return item['text']
            if role == CCM.CompletionRole:
                return CCM.GlobalScope
            if role == CCM.ScopeIndex:
                return -1
            # Return 'invalid' for other roles
            pass
        elif index.column() == CCM.Icon:
            if role == Qt.DecorationRole:
                # Show icon only if specified by a completer!
                if 'icon' in item and item['icon'] is not None:
                    return KIcon(item['icon']).pixmap(QSize(16, 16))
                pass
        elif index.column() == CCM.Arguments:
            item_args = item.get('args', None)
            if role == Qt.DisplayRole and item_args:
                return item_args
        elif index.column() == CCM.Postfix:
            item_description = item.get('description', None)
            if role == Qt.DisplayRole and item_description:
                return item_description
        elif index.column() == CCM.Prefix:
            # TODO Handle a `prefix`
            pass
        return None

    def parent(self, index):
        if index.internalId():
            return self.createIndex(0, 0, 0)
        else:
            return QModelIndex()

    def rowCount(self, parent):
        lenResultList = len(self.resultList)
        if not parent.isValid() and lenResultList:
            return 1
        elif parent.parent().isValid():
            return 0  # Do not make the model look hierarchical
        else:
            return lenResultList

    #http://api.kde.org/4.5-api/kdelibs-apidocs/interfaces/ktexteditor/html/classKTextEditor_1_1CodeCompletionModel.html#3bd60270a94fe2001891651b5332d42b
    def index(self, row, column, parent):
        if not parent.isValid():
            if row == 0:
                return self.createIndex(row, column, 0)
            else:
                return QModelIndex()
        elif parent.parent().isValid():
            return QModelIndex()
        if row < 0 or row >= len(self.resultList) or column < 0 or column >= CCM.ColumnCount:
            return QModelIndex()

        return self.createIndex(row, column, 1)

    def executeCompletionItem(self, document, word, row):
        # TODO Why this method is not called???
        pass

    def getLastExpression(self, line, operators=None):
        operators = operators or self.OPERATORS
        opmax = max(operators, key=lambda e: line.rfind(e))
        opmax_index = line.rfind(opmax)
        if line.find(opmax) != -1:
            line = line[opmax_index + 1:]
        return line.strip()

    def parseLine(self, line, col_num):
        return line[:col_num].lstrip()


class AbstractJSONFileCodeCompletionModel(AbstractCodeCompletionModel):

    FILE_PATH = None

    def __init__(self, *args, **kwargs):
        super(AbstractJSONFileCodeCompletionModel, self).__init__(*args, **kwargs)
        class_path = inspect.getfile(self.__class__)
        class_dir = os.sep.join(class_path.split(os.sep)[:-1])
        abs_file_path = os.path.join(class_dir,
                                     self.FILE_PATH)
        json_str = open(abs_file_path).read()
        self.json = json.loads(json_str)

    def getJSON(self, lastExpression, line):
        return self.json

    def completionInvoked(self, view, word, invocationType):
        line = super(AbstractJSONFileCodeCompletionModel, self).completionInvoked(view, word, invocationType)
        if not line:
            return
        lastExpression = self.getLastExpression(line)
        children = self.getChildrenInJSON(lastExpression, self.getJSON(lastExpression, line))
        if not children:
            return
        for child, attrs in children.items():
            index = self.createItemAutoComplete(text=child,
                                                category=attrs.get('category', None),
                                                args=attrs.get('args', None),
                                                description=attrs.get('description', None))
            if not index:
                continue
            self.resultList.append(index)

    def getChildrenInJSON(self, keys, json):
        if not self.SEPARATOR in keys:
            return json
        keys_split = keys.split(self.SEPARATOR)
        if keys_split and keys_split[0] in json:
            keys = self.SEPARATOR.join(keys_split[1:])
            return self.getChildrenInJSON(keys, json[keys_split[0]].get('children', None))
        return None


def reset(*args, **kwargs):
    pass
