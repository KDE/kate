# -*- coding: utf-8 -*-
'''Utils to Python: Checker Parse, Checker Pep8, Checker Pyflakes, Snippets'''

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

# This plugin originally was in this repository:
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/>
# The original author of the pep8 and pyflakes checker is Alejandro Blanco <alejandro.b.e@gmail.com>

from libkatepate.errors import needs_packages

from python_snippets import *
from python_checkers.parse_checker import *

msg_error = ""

try:
    needs_packages({"pep8": "1.4.2"})
    from python_checkers.pep8_checker import *
except ImportError as e:
    msg_error += e.message + "\n"
finally:
    try:
        needs_packages({"pyflakes": "0.6.1"})
        from python_checkers.pyflakes_checker import *
    except ImportError as e:
        msg_error += e.message
        raise ImportError(msg_error)
