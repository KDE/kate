# -*- coding: utf-8 -*-
'''Utils to Python: Autocomplete, Checker Parse, Checker Pep8, Checker Pyflakes, Snippets'''

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

NEED_PACKAGES = {}

try:
    import pep8
except ImportError:
    NEED_PACKAGES["pep8"] = "1.4.2"

try:
    import pyflakes
except ImportError:
    NEED_PACKAGES["pyflakes"] = "0.6.1"


try:
    import pysmell
except ImportError:
    NEED_PACKAGES["pysmell"] = "0.0.2"


if not "pysmell" in NEED_PACKAGES:
    try:
        import pyplete
    except ImportError:
        NEED_PACKAGES["pyplete"] = "0.0.2"


from python_snippets import *
from python_checkers.parse_checker import *

if not "pep8" in NEED_PACKAGES:
    from python_checkers.pep8_checker import *

if not "pyflakes" in NEED_PACKAGES:
    from python_checkers.pyflakes_checker import *

if not "pysmell" in NEED_PACKAGES and not "pyplete" in NEED_PACKAGES:
    from python_autocomplete.autocomplete import *

if NEED_PACKAGES:
    msg = "You need install the next packages:\n"
    for package in NEED_PACKAGES:
        msg += "\t\t%(package)s. Use easy_install %(package)s==%(version)s" % {'package': package,
                                                                               'version': NEED_PACKAGES[package]}
    raise ImportError(msg)
