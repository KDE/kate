# -*- coding: utf-8 -*-
'''Utils to JavaScript: Autocomplete, Autocomplete jQuery, Checker JSLint, Pretty JSON, Snippets'''

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
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/jste_plugins/>
# The original author of the jslint checker is Alejandro Blanco <alejandro.b.e@gmail.com>

NEED_PACKAGES = {}

try:
    import simplejson
except ImportError:
    NEED_PACKAGES["simplejson"] = "3.0.7"

try:
    import pyjslint
except ImportError:
    NEED_PACKAGES["pyjslint"] = "0.3.3"


from js_snippets import *

if not "simplejson" in NEED_PACKAGES:
    from js_autocomplete import *
    from json_pretty import *

if not "pyjslint" in NEED_PACKAGES:
    from jslint import *


if NEED_PACKAGES:
    msg = "You need install the next packages:\n"
    for package in NEED_PACKAGES:
        msg += "\t\t%(package)s. Use easy_install %(package)s==%(version)s" % {'package': package,
                                                                               'version': NEED_PACKAGES[package]}
    raise ImportError(msg)
