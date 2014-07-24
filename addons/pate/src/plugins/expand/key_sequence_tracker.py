# -*- coding: utf-8 -*-
#
# This file is part of Kate's Expand plugin.
#
# Copyright (C) 2013 Alex Turbov <i.zaufi@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

'''Class to track key sequences'''

import kate

from PyKDE4.ktexteditor import KTextEditor

from .decorators import *
from .jinja_stuff import render_jinja_template


class KeySequenceTracker:
    TRIGGER_CHAR = ';'

    def __init__(self):
        self.next_expected_pos = True                         # Reset needed!
        self.reset()


    def reset(self):
        '''Drop currently collected track'''
        if self.next_expected_pos is not None:
            kate.qDebug('@KeySequenceTracker: ---[resetting...]---')
            self.sequence = ''                              # Characters sequence storage
            self.next_expected_pos = None


    def track_input(self, char, cursor):
        assert('parameter expected to be a string' and isinstance(char, str))

        #kate.qDebug('@KeySequenceTracker: input="{}"'.format(char))
        #kate.qDebug('@KeySequenceTracker: curr-pos=({},{})'.format(cursor.line(),cursor.column()))
        #if self.next_expected_pos is not None:
            #kate.qDebug('@KeySequenceTracker: next-pos=({},{})'.format(self.next_expected_pos.line(),self.next_expected_pos.column()))

        if len(char) != 1:
            self.reset()                                    # Looks like copy-n-pasted text... drop it!
        elif char == KeySequenceTracker.TRIGGER_CHAR:
            if self.next_expected_pos is not None:          # Is tracking already enabled?
                self._dispatch()                            # Yep, then this is a final char... Handle it!
                self.reset()                                # Ready for next key sequence
            else:
                kate.qDebug('@KeySequenceTracker: ---[start tracking key sequence]---')
                self.next_expected_pos = KTextEditor.Cursor(cursor.line(), cursor.column() + 1)
            self.moved = False
        elif self.next_expected_pos is not None:            # Do we record?
            # ATTENTION Ugly PyKDE do not export some vital functions!
            # Like relation operators for types (Cursor and Range for example)...
            is_equal_cursors = self.next_expected_pos.line() == cursor.line() \
              and self.next_expected_pos.column() == cursor.column()
            if is_equal_cursors:
                self.sequence += char                       # Yeah! Just collect a one more char
                self.next_expected_pos = KTextEditor.Cursor(cursor.line(), cursor.column() + 1)
            else:
                kate.qDebug('@KeySequenceTracker: ---[next-pos != cursor]---')
                self.reset()


    def track_cursor(self, cursor):
        kate.qDebug('@KeySequenceTracker: moved to ({},{})'.format(cursor.line(),cursor.column()))
        if self.next_expected_pos is not None \
          and ( \
                self.next_expected_pos.line() != cursor.line() \
              or (self.next_expected_pos.column() + 1) != cursor.column() \
          ):
            kate.qDebug('@KeySequenceTracker: moved from ({},{})'.format(self.next_expected_pos.line(),self.next_expected_pos.column()))
            self.reset()


    def _dispatch(self):
        kate.qDebug('@KeySequenceTracker: collected key sequence "{}"'.format(self.sequence))
        view = kate.activeView()
        cursor = view.cursorPosition()
        word_range = KTextEditor.Range(
            cursor.line()
          , cursor.column() - len(self.sequence) - 2
          , cursor.line()
          , cursor.column()
          )
        document = view.document()
        assert('Sanity check' and view is not None)
        mimeType = document.mimeType()
        expansions = getExpansionsFor(mimeType)

        if not hasattr(dynamic, 'registered_handlers') or mimeType not in dynamic.registered_handlers:
            return

        assert('Sanity check' and isinstance(dynamic.registered_handlers[mimeType], list))
        for func in dynamic.registered_handlers[mimeType]:
            assert('Sanity check' and inspect.isfunction(func) and hasattr(func, 'match_regex'))
            match = func.match_regex.search(self.sequence)
            if match is not None:
                result = func(self.sequence, match)
                self._inject_expand_result(document, result, word_range, func)
                return
        # NOTE If no such <del>cheat-code</del> key sequence registered, just do nothing!
        # (nothing == even do not show any warning!)
        # ... just like w/ XML: unknown tags are ignored ;-)


    def _inject_expand_result(self, document, result, word_range, func):
        '''TODO Catch exceptions from provided function'''

        # Check what type of expand function it was
        if hasattr(func, 'template'):
            result = render_jinja_template(func.template, result)

        kate.qDebug('result={}'.format(repr(result)))
        assert(isinstance(result, str))

        #
        with kate.makeAtomicUndo(document):
            document.removeText(word_range)

            kate.qDebug('Expanded text:\n{}'.format(result))
            # Check if expand function requested a TemplateInterface2 to render
            # result content...
            if hasattr(func, 'use_template_iface') and func.use_template_iface:
                # Use TemplateInterface2 to insert a code snippet
                kate.qDebug('TI2 requested!')
                ti2 = document.activeView().templateInterface2()
                if ti2 is not None:
                    ti2.insertTemplateText(word_range.start(), result, {}, None)
                    return
                # Fallback to default (legacy) way...

            # Just insert text :)
            document.insertText(word_range.start(), result)
