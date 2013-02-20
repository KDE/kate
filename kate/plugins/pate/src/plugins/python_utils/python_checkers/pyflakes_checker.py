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

from pyflakes.checker import Checker
from pyflakes.messages import Message

from libkatepate.errors import showOk, showErrors

from python_checkers.all_checker import checkAll
from python_checkers.utils import canCheckDocument
from python_settings import KATE_ACTIONS


def pyflakes(codeString, filename):
    """
    Check the Python source given by C{codeString} for flakes.
    """
    # First, compile into an AST and handle syntax errors.
    try:
        tree = compile(codeString, filename, "exec", _ast.PyCF_ONLY_AST)
    except SyntaxError, value:
        msg = value.args[0]
        lineno = value.lineno
        # If there's an encoding problem with the file, the text is None.
        if value.text is None:
            # Avoid using msg, since for the only known case, it contains a
            # bogus message that claims the encoding the file declared was
            # unknown.
            msg = "Problem decoding source"
            lineno = 1
        error = Message(filename, lineno)
        error.message = msg + "%s"
        error.message_args = ""
        return [error]
    else:
        # Okay, it's syntactically valid.  Now check it.
        w = Checker(tree, filename)
        w.messages.sort(lambda a, b: cmp(a.lineno, b.lineno))
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

    path = unicode(currentDocument.url().path())
    mark_key = '%s-pyflakes' % path

    text = unicode(currentDocument.text())
    errors = pyflakes(text.encode('utf-8', 'ignore'), path)
    errors_to_show = []

    if len(errors) == 0:
        showOk("Pyflakes Ok")
        return

    # Prepare errors found for painting
    for error in errors:
        errors_to_show.append({
            "message": error.message % error.message_args,
            "line": error.lineno,
            })

    showErrors('Pyflakes Errors:', errors_to_show,
                mark_key, currentDocument,
                move_cursor=move_cursor)
