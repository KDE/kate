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

# This core originally was in this repository:
# <https://github.com/goinnn/Kate-plugins/blob/master/kate_plugins/autopate.py>

import inspect
import os

from PyKDE4.kdeui import KIcon
from PyKDE4.ktexteditor import KTextEditor
from PyQt4.QtCore import QModelIndex, QSize, Qt


class AbstractCodeCompletionModel(KTextEditor.CodeCompletionModel):

    TITLE_AUTOCOMPLETION = 'Autopate'
    MIMETYPES = []
    OPERATORS = []
    SEPARATOR = '.'

    def __init__(self, model, resultList=None):
        self.model = model
        super(AbstractCodeCompletionModel, self).__init__(model)
        self.resultList = []

    roles = {
        KTextEditor.CodeCompletionModel.CompletionRole:
            KTextEditor.CodeCompletionModel.FirstProperty |
            KTextEditor.CodeCompletionModel.Public |
            KTextEditor.CodeCompletionModel.LastProperty |
            KTextEditor.CodeCompletionModel.Prefix,
        KTextEditor.CodeCompletionModel.ScopeIndex: 0,
        KTextEditor.CodeCompletionModel.MatchQuality: 10,
        KTextEditor.CodeCompletionModel.HighlightingMethod: None,
        KTextEditor.CodeCompletionModel.InheritanceDepth: 0,
    }

    @classmethod
    def createItemAutoComplete(cls, text, category='unknown', args=None, description=None):
        icon_converter = {'package': 'code-block',
                          'module': 'code-context',
                          'unknown': 'unknown',
                          'constant': 'code-variable',
                          'class': 'code-class',
                          'function': 'code-function',
                          'pointer': 'unknown'}
        max_description = 50
        if description and len(description) > max_description:
            description = description.strip()
            description = '%s...' % description[:max_description]
        return {'text': text,
                'icon': icon_converter[category],
                'category': category,
                'args': args or '',
                'type': category,
                'description': description or ''}

    def completionInvoked(self, view, word, invocationType):
        line_start = word.start().line()
        line_end = word.end().line()
        column_end = word.end().column()
        self.resultList = []
        self.invocationType = invocationType
        if line_start != line_end:
            return None
        mimetype = unicode(view.document().mimeType())
        if not mimetype in self.MIMETYPES:
            return None
        doc = view.document()
        line = unicode(doc.line(line_start))
        if not line:
            return line
        return self.parseLine(line, column_end)

    def data(self, index, role):
        if not index.parent().isValid():
            return self.TITLE_AUTOCOMPLETION
        item = self.resultList[index.row()]
        if index.column() == KTextEditor.CodeCompletionModel.Name:
            if role == Qt.DisplayRole:
                return item['text']
            try:
                return self.roles[role]
            except KeyError:
                pass
        elif index.column() == KTextEditor.CodeCompletionModel.Icon:
            if role == Qt.DecorationRole:
                return KIcon(item["icon"]).pixmap(QSize(16, 16))
        elif index.column() == KTextEditor.CodeCompletionModel.Arguments:
            item_args = item.get("args", None)
            if role == Qt.DisplayRole and item_args:
                return item_args
        elif index.column() == KTextEditor.CodeCompletionModel.Postfix:
            item_description = item.get("description", None)
            if role == Qt.DisplayRole and item_description:
                return item_description
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
        if row < 0 or row >= len(self.resultList) or column < 0 or column >= KTextEditor.CodeCompletionModel.ColumnCount:
            return QModelIndex()

        return self.createIndex(row, column, 1)

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
        from simplejson import loads
        self.json = loads(json_str)

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
