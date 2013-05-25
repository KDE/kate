# -*- coding: utf-8 -*-
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

from PyKDE4.kdecore import i18n

from libkatepate import text

JS_MENU = 'JavaScript'

KATE_ACTIONS = {'insertReady': {'text': 'jQuery Ready',
                                'shortcut': 'Ctrl+J',
                                'menu': JS_MENU,
                                'icon': None},
                'checkJslint': {'text': 'JSLint',
                                'shortcut': 'Alt+9',
                                'menu': JS_MENU,
                                'icon': None},
                'togglePrettyJsonFormat': {'text': i18n('Pretty JSON'),
                                           'shortcut': 'Ctrl+Alt+J',
                                           'menu': JS_MENU,
                                           'icon': None},
                }

KATE_CONFIG = {'name': 'js_utils',
               'fullName': i18n('JavaScript Utils'),
               'icon': 'application-x-javascript'}
_INDENT_JSON_CFG = 'JSUtils:indentJSON'
_ENCODING_JSON_CFG = 'JSUtils:encodingJSON'
_JSLINT_CHECK_WHEN_SAVE = 'JSUtils:jslintCheckSave'
_ENABLE_JS_AUTOCOMPLETE = 'JSUtils:jsAutoComplete'
_ENABLE_JQUERY_AUTOCOMPLETE = 'JSUtils:JQueryAutoComplete'
_ENABLE_TEXT_JQUERY = 'JSUtils:textJQuery'
DEFAULT_INDENT_JSON = 4
DEFAULT_ENCODING_JSON = 'utf-8'
DEFAULT_CHECK_JSLINT_WHEN_SAVE = True
DEFAULT_ENABLE_JS_AUTOCOMPLETE = True
DEFAULT_ENABLE_JQUERY_AUTOCOMPLETE = True
DEFAULT_TEXT_JQUERY = """<script type="text/javascript">
\t(function($){
\t\t$(document).ready(function () {
\t\t\t$("%s").click(function(){
\t\t\t\t// Write here
\t\t\t});
\t\t});
\t\t})(jQuery);
</script>
""" % text.TEXT_TO_CHANGE


# kate: space-indent on; indent-width 4;
