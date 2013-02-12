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
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/autocomplete/autocomplete.py>


import sys

import kate

from PyKDE4.ktexteditor import KTextEditor

from libkatepate.autocomplete import AbstractCodeCompletionModel, reset
from libkatepate.session import get_session
from python_settings import PYTHON_AUTOCOMPLETE_ENABLED
from pyplete import PyPlete
from python_autocomplete.parse import (import_complete,
                                       from_first_imporable,
                                       from_other_imporables,
                                       from_complete)

global pyplete
global python_path
pyplete = None
python_path = []


class PythonCodeCompletionModel(AbstractCodeCompletionModel):

    TITLE_AUTOCOMPLETION = "Python Auto Complete"
    MIMETYPES = ['text/plain', 'text/x-python']
    OPERATORS = ["=", " ", "[", "]", "(", ")", "{", "}", ":", ">", "<",
                 "+", "-", "*", "/", "%", " and ", " or ", ","]

    def __init__(self, session=None, *args, **kwargs):
        super(PythonCodeCompletionModel, self).__init__(*args, **kwargs)
        self.session = session

    @classmethod
    def getPythonPath(cls, recalculeSession=False):
        global python_path
        if python_path and not recalculeSession:
            return python_path
        python_path = sys.path
        try:
            from pyte_plugins.autocomplete import autocomplete_path
            doc = kate.activeDocument()
            view = doc.activeView()
            python_path = autocomplete_path.path(recalculeSession, doc, view) + python_path
        except ImportError:
            pass
        return python_path

    def completionInvoked(self, view, word, invocationType):
        line = super(PythonCodeCompletionModel, self).completionInvoked(view,
                                                       word, invocationType)
        session = get_session()
        if self.session != session:
            self.setSession(session)
        if line is None:
            return
        is_auto = False
        line_rough = line
        if 'from' in line or 'import' in line:
            is_auto = self.autoCompleteImport(view, word, line)
        line = self.getLastExpression(line)
        if not is_auto and line:
            is_auto = self.autoCompleteLine(view, word, line)
        if not is_auto and line and line_rough and not self.SEPARATOR in line_rough:
            is_auto = self.autoCompleteFile(view, word, line)

    def parseLine(self, line, col_num):
        line = super(PythonCodeCompletionModel, self).parseLine(line, col_num)
        if "'" in line or '"' in line:
            return line
        if ";" in line:
            return self.parseLine(line.split(";")[-1], col_num)
        return line

    def autoCompleteImport(self, view, word, line):
        mfb = from_first_imporable.match(line) or import_complete.match(line)
        if mfb:
            return pyplete.get_importables_top_level(self.resultList)
        mfom = from_other_imporables.match(line)
        if mfom:
            imporable, subimporables = mfom.groups()
            if not subimporables:
                subimporables = []
            else:
                subimporables = subimporables.split(self.SEPARATOR)[:-1]
            return pyplete.get_importables_rest_level(self.resultList, imporable,
                                                      subimporables, into_module=False)
        mfc = from_complete.match(line)
        if mfc:
            imporable, subimporables, import_imporable = mfc.groups()
            if subimporables:
                subimporables = subimporables.split(self.SEPARATOR)
            else:
                subimporables = []
            return pyplete.get_importables_rest_level(self.resultList, imporable,
                                                      subimporables, into_module=True)
        return False

    def autoCompleteLine(self, view, word, line):
        try:
            last_dot = line.rindex(self.SEPARATOR)
            line = line[:last_dot]
        except ValueError:
            pass
        text = self._parseText(view, word, line)
        return self.getImportablesFromLine(self.resultList, text, line)

    def autoCompleteFile(self, view, word, line):
        text = self._parseText(view, word, line)
        return self.getImportablesFromText(self.resultList, text)

    def getImportablesFromLine(self, list_autocomplete, text,
                               code_line, text_info=True):
        try:
            return pyplete.get_importables_from_line(list_autocomplete, text,
                                                     code_line, text_info)
        except SyntaxError, e:
            self.treatmentException(e)
        return False

    def getImportablesFromText(self, list_autocomplete, text):
        try:
            return pyplete.get_importables_from_text(list_autocomplete, text)
        except SyntaxError, e:
            self.treatmentException(e)
        return False

    def treatmentException(self, e):
        if self.invocationType == KTextEditor.CodeCompletionModel.AutomaticInvocation:
            return
        f = e.filename or ''
        text = e.text
        line = e.lineno
        message = 'There was a syntax error in this file:'
        if f:
            message = '%s\n  * file: %s' % (message, f)
        if text:
            message = '%s\n  * text: %s' % (message, text)
        if line:
            message = '%s\n  * line: %s' % (message, line)
        kate.gui.popup(message, 2, icon='dialog-warning', minTextWidth=200)

    def setSession(self, session):
        global pyplete
        if pyplete and session == self.session:
            return
        self.session = session
        pyplete = PyPlete(PythonCodeCompletionModel.createItemAutoComplete,
                          PythonCodeCompletionModel.getPythonPath(recalculeSession=self.session))

    def _parseText(self, view, word, line):
        doc = view.document()
        text = unicode(doc.text())
        text = text.encode('utf-8', 'ignore')
        text_list = text.split("\n")
        raw, column = word.start().position()
        line = text_list[raw]
        if ";" in line and not "'" in line and not '"' in line:
            text_list[raw] = ';'.join(text_list[raw].split(";")[:-1])
        else:
            del text_list[raw]
        text = '\n'.join(text_list)
        line = line.strip()
        return text


@kate.init
@kate.viewCreated
def createSignalAutocompleteDocument(view=None, *args, **kwargs):
    # https://launchpad.net/ubuntu/precise/+source/pykde4
    # https://launchpad.net/ubuntu/precise/+source/pykde4/4:4.7.97-0ubuntu1/+files/pykde4_4.7.97.orig.tar.bz2
    # http://doc.trolltech.com/4.6/qabstractitemmodel.html
    # http://gitorious.org/kate/kate/blobs/a17eb928f8133528a6194b7e788ab7a425ef5eea/ktexteditor/codecompletionmodel.cpp
    # http://code.google.com/p/lilykde/source/browse/trunk/frescobaldi/python/frescobaldi_app/mainapp.py#1391
    # http://api.kde.org/4.0-api/kdelibs-apidocs/kate/html/katecompletionmodel_8cpp_source.html
    # https://svn.reviewboard.kde.org/r/1640/diff/?expand=1
    if not PYTHON_AUTOCOMPLETE_ENABLED:
        return
    view = view or kate.activeView()
    global pyplete
    session = get_session()
    codecompletationmodel.setSession(session)
    cci = view.codeCompletionInterface()
    cci.registerCompletionModel(codecompletationmodel)


if PYTHON_AUTOCOMPLETE_ENABLED:
    session = None
    codecompletationmodel = PythonCodeCompletionModel(session, kate.application)
    codecompletationmodel.modelReset.connect(reset)
