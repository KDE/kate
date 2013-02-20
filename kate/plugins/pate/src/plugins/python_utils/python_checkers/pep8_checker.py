# -*- coding: utf-8 -*-
# Copyright (c) 2013 by Pablo Martín <goinnn@gmail.com> and
# Alejandro Blanco <alejandro.b.e@gmail.com>
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
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/check_plugins/pep8_plugins.py>

import sys

import kate
import pep8

from libkatepate.errors import showOk, showErrors

from python_checkers.all_checker import checkAll
from python_checkers.utils import canCheckDocument
from python_settings import KATE_ACTIONS, IGNORE_PEP8_ERRORS

OLD_PEP8_VERSIONS = ['1.0.1', '1.1', '1.2']


if pep8.__version__ not in OLD_PEP8_VERSIONS:
    class KateReport(pep8.BaseReport):

        def __init__(self, options):
            super(KateReport, self).__init__(options)
            self.error_list = []
            self.ignore = options.ignore

        def error(self, line_number, offset, text, check):
            code = super(KateReport, self).error(line_number, offset, text, check)
            self.error_list.append([line_number, offset, text[0:4], text])
            return code

        def get_errors(self):
            """
            Get the errors, and reset the checker
            """
            result, self.error_list = self.error_list, []
            return result
else:
    class StoreErrorsChecker(pep8.Checker):

        def __init__(self, *args, **kwargs):
            super(StoreErrorsChecker, self).__init__(*args, **kwargs)
            self.error_list = []

        def report_error(self, line_number, offset, text, check):
            """
            Store the error
            """
            self.file_errors += 1
            error_code = text[0:4]
            if not pep8.ignore_code(error_code):
                self.error_list.append([line_number, offset, error_code, text])

        def get_errors(self):
            """
            Get the errors, and reset the checker
            """
            result, self.error_list = self.error_list, []
            self.file_errors = 0
            return result


def saveFirst():
    kate.gui.popup('You must save the file first', 3,
                    icon='dialog-warning', minTextWidth=200)


@kate.action(**KATE_ACTIONS['checkPep8'])
def checkPep8(currentDocument=None, refresh=True):
    """Check the pep8 errors of the document"""
    if not canCheckDocument(currentDocument):
        return
    if refresh:
        checkAll.f(currentDocument, ['checkPep8'],
                 exclude_all=not currentDocument)
    move_cursor = not currentDocument
    currentDocument = currentDocument or kate.activeDocument()

    if currentDocument.isModified():
        saveFirst()
        return
    path = unicode(currentDocument.url().path())
    if not path:
        saveFirst()
        return
    mark_key = '%s-pep8' % unicode(currentDocument.url().path())
    # Check the file for errors with PEP8
    sys.argv = [path]
    pep8.process_options([path])
    if pep8.__version__ in OLD_PEP8_VERSIONS:
        checker = StoreErrorsChecker(path)
        pep8.options.ignore = IGNORE_PEP8_ERRORS
        checker.check_all()
        errors = checker.get_errors()
    else:
        checker = pep8.Checker(path, reporter=KateReport, ignore=IGNORE_PEP8_ERRORS)
        checker.check_all()
        errors = checker.report.get_errors()
    if len(errors) == 0:
        showOk('Pep8 Ok')
        return
    errors_to_show = []
    # Paint errors found
    for error in errors:
        errors_to_show.append({
            "line": error[0],
            "column": error[1] + 1,
            "message": error[3],
            })
    showErrors('Pep8 Errors:',
               errors_to_show,
               mark_key,
               currentDocument,
               move_cursor=move_cursor)
