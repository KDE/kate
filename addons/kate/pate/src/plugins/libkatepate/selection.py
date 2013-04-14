#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2010-2012 by Alex Turbov <i.zaufi@gmail.com> and
# Copyright 2013 by Pablo Martín <goinnn@gmail.com>
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

import kate
from PyKDE4.ktexteditor import KTextEditor

''' Reusable code for Kate/Pâté plugins: selection mode constants

    ... just to introduce separate `namespace`
    NOTE Is there (in Python) a better way to do it?
'''

NORMAL = False
BLOCK = True


def setSelectionFromCurrentPosition(start, end, pos=None):
    view = kate.activeView()
    pos = pos or view.cursorPosition()
    cursor1 = KTextEditor.Cursor(pos.line() + start[0], pos.column() + start[1])
    cursor2 = KTextEditor.Cursor(pos.line() + end[0], pos.column() + end[1])
    view.setSelection(KTextEditor.Range(cursor1, cursor2))
    view.setCursorPosition(cursor1)
