# -*- coding: utf-8 -*-
#
# Copyright 2010-2012 by Alex Turbov <i.zaufi@gmail.com>
#
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
#

''' Reusable code for Kate/Pâté plugins: decorators for actions '''

import functools
import kate
from libkatepate import ui
from libkatepate import common

def append_constraint(action, constraint):
    if not hasattr(action, 'constraints'):
        action.constraints = []
    action.constraints.append(constraint)


def check_constraints(action):
    ''' Decorator to evaluate constraints assigned to a given action '''
    def checker(*args, **kw):
        document = kate.activeDocument()
        # TODO Why this shit^W`if` doesn't work? WTF?!
        #if not hasattr(action, 'constraints') or reduce(lambda i, j: i(document) and j(document), action.constraints)():
        if hasattr(action, 'constraints'):
            for c in action.constraints:
                if not c(document):
                    return
        return action(*args, **kw)
    return checker


def restrict_doc_type(*doc_types):
    def restrict_doc_type_decorator(action):
        # TODO Investgation required why in opposite params order
        # keyword arguments can't pass through `partial` binder
        # WTF?? WTF!!
        def doc_type_checker(doc_types, document):
            doc_type = document.highlightingMode()
            if doc_type not in doc_types:
                ui.popup(
                    "Alert"
                  , "This action have no sense for " + doc_type + " documents!"
                  , "face-wink"
                  )
                return False
            return True
        binded_predicate = functools.partial(doc_type_checker, doc_types)
        append_constraint(action, binded_predicate)
        return action
    return restrict_doc_type_decorator


def comment_char_must_be_known(dummy = None):
    def comment_char_known_decorator(action):
        # TODO Same shit here as for restrict_doc_type!
        def comment_char_checker(dummy, document):
            doc_type = document.highlightingMode()
            result = common.isKnownCommentStyle(doc_type)
            if not result:
                ui.popup(
                    "Oops!"
                  , "Don't know how comments look like for " + doc_type + " documents!"
                  , "face-uncertain"
                  )
            return result
        binded_predicate = functools.partial(comment_char_checker, dummy)
        append_constraint(action, binded_predicate)
        return action
    return comment_char_known_decorator


def selection_mode(selectionMode):
    ''' Restrict action to given selection mode

        ALERT In block mode it is Ok to have cursor position after the line end!
        This could lead to 'index is out of string' exceptions if algorithm is a little rough :-)
        So it is why this constraint needed :)
    '''
    def selection_mode_decorator(action):
        def selection_mode_checker(selectionMode, document):
            view = document.activeView()
            result = selectionMode == view.blockSelection()
            if not result:
                if selectionMode:
                    mode = 'block'
                else:
                    mode = 'normal'
                ui.popup(
                    "Oops!"
                  , "This operation is for %s selection mode!" % mode
                  , "face-sad"
                  )
            return result
        binded_predicate = functools.partial(selection_mode_checker, selectionMode)
        append_constraint(action, binded_predicate)
        return action
    return selection_mode_decorator


def has_selection(selectionState):
    def has_selection_decorator(action):
        def has_selection_checker(selectionState, document):
            view = document.activeView()
            result = selectionState == view.selection()
            print("*** has_selection: result=%s" % repr(result))
            if not result:
                if not selectionState:
                    should = "n't"
                else:
                    should = ''
                ui.popup(
                    "Oops!"
                  , "Document should%s have selection to perform this operation" % should
                  , "face-sad"
                  )
            return result
        binded_predicate = functools.partial(has_selection_checker, selectionState)
        append_constraint(action, binded_predicate)
        return action
    return has_selection_decorator


# TODO All decorators are look same...
# It seems we need another one decorator to produce decorators of decorators :-)
