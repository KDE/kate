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
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/check_plugins/pyflakes_plugins.py>

import _ast

import kate
import re
import sys

from PyKDE4.kdecore import i18n

from pyflakes import __version__ as pyflakesVersion
from pyflakes.checker import Checker
from pyflakes.messages import Message

from libkatepate.errors import showOk, showErrors

from python_checkers.all_checker import checkAll
from python_checkers.utils import canCheckDocument
from python_settings import KATE_ACTIONS

first_two_lines = re.compile(r'(?:[^\n]*\n){0,2}')
encoding_line = re.compile(r"coding[=:]\s*(?P<encoding>[-\w.]+)")


def is_old_pyflake_version():
    version_split = pyflakesVersion.split('.')
    if len(version_split) == 3:
        try:
            version_major = int(version_split[0])
            version_minor = int(version_split[1])
            return version_major == 0 and version_minor <= 6
        except ValueError:
            pass
    return False


def pyflakes(codeString, filename):
    """
    Check the Python source given by C{codeString} for flakes.
    """
    # First, compile into an AST and handle syntax errors.
    try:
        if sys.version_info.major == 2 and codeString:
            lines_match = first_two_lines.match(codeString)
            if lines_match:
                lines = lines_match.group(0)
                encoding_match = encoding_line.search(lines)
                if encoding_match:
                    encoding = encoding_match.groupdict().get('encoding', None)
                    if encoding:
                        codeString = codeString.encode(encoding)
        tree = compile(codeString, filename, "exec", _ast.PyCF_ONLY_AST)
    except SyntaxError as value:
        msg = value.args[0]
        # If there's an encoding problem with the file, the text is None.
        if value.text is None:
            # Avoid using msg, since for the only known case, it contains a
            # bogus message that claims the encoding the file declared was
            # unknown.
            msg = i18n("Problem decoding source")
            value.lineno = 1
        if not is_old_pyflake_version():
            error = Message(filename, value)
        else:
            error = Message(filename, value.lineno)
        error.message = msg + "%s"
        error.message_args = ""
        return [error]
    else:
        # Okay, it's syntactically valid.  Now check it.
        w = Checker(tree, filename)
        return w.messages


@kate.action(**KATE_ACTIONS['checkPyflakes'])
def checkPyflakes(currentDocument=None, refresh=True):
    """Check the pyflakes errors of the document"""
    if not canCheckDocument(currentDocument):
        return
    if refresh:
        checkAll.f(currentDocument, ['checkPyflakes'],
                   exclude_all=not currentDocument)
    move_cursor = not currentDocument
    currentDocument = currentDocument or kate.activeDocument()

    path = currentDocument.url().path()
    mark_key = '%s-pyflakes' % path

    text = currentDocument.text()
    errors = pyflakes(text, path)
    errors_to_show = []

    if len(errors) == 0:
        showOk(i18n("Pyflakes Ok"))
        return

    # Prepare errors found for painting
    for error in errors:
        error_to_show = {
            "message": error.message % error.message_args,
            "line": error.lineno,
        }
        if getattr(error, 'col', None) is not None:
            error_to_show['column'] = error.col + 1
        errors_to_show.append(error_to_show)

    showErrors(i18n('Pyflakes Errors:'), errors_to_show,
               mark_key, currentDocument,
               move_cursor=move_cursor)

# kate: space-indent on; indent-width 4;
