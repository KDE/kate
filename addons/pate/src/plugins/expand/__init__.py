# -*- coding: utf-8 -*-
# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
# Copyright (C) 2013 Alex Turbov <i.zaufi@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) version 3.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

'''User-defined text expansions.


Each text expansion is a simple function which must return a string.
This string will be inserted into a document by the expandAtCursor action.
For example if you have a function "foo" in the all.expand file
which is defined as:

def foo:
  return 'Hello from foo!'

after typing "foo", the action will replace "foo" with "Hello from foo!".
The expansion function may have parameters as well. For example, the
text_x-c++src.expand file contains the "cl" function which creates a class
(or class template). This will expand "cl(test)" into:

/**
 * \\brief Class \c test
 */
class test
{
public:
    /// Default constructor
    explicit test()
    {
    }
    /// Destructor
    virtual ~test()
    {
    }
};

but "cl(test, T1, T2, T3)" will expand to:

/**
 * \\brief Class \c test
 */
template <typename T1, typename T2, typename T3>
class test
{
public:
    /// Default constructor
    explicit test()
    {
    }
    /// Destructor
    virtual ~test()
    {
    }
};
'''

import kate.view

from PyKDE4.kdecore import i18nc

from .decorators import *
from .jinja_stuff import jinja_environment_configurator
from .udf import *

# NOTE Do not show the following imports as part of our API!
from .expands_completion_model import ExpandsCompletionModel as _ExpandsCompletionModel
from .key_sequence_tracker import KeySequenceTracker as _KeySequenceTracker


_expands_completion_model = None
_input_tracker = None


@kate.action
def expandAtCursorAction():
    expandUDFAtCursor()


def _reset(*args, **kwargs):
    global _expands_completion_model
    if _expands_completion_model is not None:
        _expands_completion_model.reset()


@kate.init
def _on_load():
    global _input_tracker
    assert(_input_tracker is None)
    _input_tracker = _KeySequenceTracker()

    global _expands_completion_model
    assert(_expands_completion_model is None)
    _expands_completion_model = _ExpandsCompletionModel(kate.application)
    _expands_completion_model.modelReset.connect(_reset)
    # Set completion model for all already existed views
    # (cuz the plugin can be loaded in the middle of editing session)
    for doc in kate.documentManager.documents():
        for view in doc.views():
            cci = view.codeCompletionInterface()
            cci.registerCompletionModel(_expands_completion_model)


@kate.unload
def _on_unoad():
    global _input_tracker
    assert(_input_tracker is not None)
    _input_tracker = None

    global _expands_completion_model
    assert(_expands_completion_model is not None)
    for doc in kate.documentManager.documents():
        for view in doc.views():
            cci = view.codeCompletionInterface()
            cci.unregisterCompletionModel(_expands_completion_model)
    _expands_completion_model = None


@kate.viewCreated
def _createSignalAutocompleteExpands(view):
    global _expands_completion_model
    if view:
        cci = view.codeCompletionInterface()
        cci.registerCompletionModel(_expands_completion_model)


@kate.viewChanged
@kate.view.contextMenuAboutToShow
@kate.view.focusOut
@kate.view.selectionChanged
@kate.view.viewInputModeChanged
@kate.view.viewModeChanged
def _resetTracker0(*args, **kw):
    global _input_tracker
    if _input_tracker is not None:
        _input_tracker.reset()


@kate.view.cursorPositionChanged
def _moved(view, cursor):
    global _input_tracker
    if _input_tracker is not None:
        _input_tracker.track_cursor(cursor)


@kate.view.textInserted
def _feedTracker(view, cursor, text):
    global _input_tracker
    kate.qDebug('@input={},c=({},{})'.format(text, cursor.line(), cursor.column()))
    if _input_tracker is not None:
        _input_tracker.track_input(text, cursor)
