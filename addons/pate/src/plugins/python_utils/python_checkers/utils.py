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


def is_mymetype_python(doc, text_plain=False):
    mimetype = doc.mimeType()
    if mimetype == 'text/x-python':
        return True
    elif mimetype == 'text/plain' and text_plain:
        return True
    return False


def canCheckDocument(doc, text_plain=False):
    return not doc or (is_mymetype_python(doc, text_plain) and
                       not doc.isModified())

# kate: space-indent on; indent-width 4;
