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

from __future__ import absolute_import

import kate

from libkatepate import text

class SettingType:
    def __init__(self, default, alternate):
        if isinstance(default, bool):
            self.getter = 'isChecked'
            self.setter = 'setChecked'
        elif isinstance(default, int):
            if alternate:
                self.getter = 'currentIndex'
                self.setter = 'setCurrentIndex'
            else:
                self.getter = 'value'
                self.setter = 'setValue'
        elif isinstance(default, str):
            if alternate:
                self.getter = 'toPlainText'
                self.setter = 'setPlainText'
            else:
                self.getter = 'text'
                self.setter = 'setText'
        else:
            raise ValueError('can’t cope with default {} of type {}'.format(default, type))

class Setting:
    """Abstract setting with default value, aware of Qt classes to bind to"""
    def __init__(self, key, default, choices=None, area=False):
        self.key = key
        self.default = default
        self.type = SettingType(default, area or choices)
        self.choices = choices
        self.conf = kate.configuration.root.setdefault('js_utils', {})

    def lookup(self):
        """Retrieves value from Kate’s config"""
        return self.conf.get(self.key, self.default)

class BoundSetting(Setting):
    """Binding of Setting to a concrete widget"""
    def __init__(self, setting, widget):
        self.key = setting.key
        self.default = setting.default
        self.type = setting.type
        self.choices = setting.choices
        self.conf = setting.conf

        self.getter = getattr(widget, self.type.getter)
        self.setter = getattr(widget, self.type.setter)
        for choice in self.choices or []:
            widget.addItem(choice)

        self.setter(self.lookup())

    def reset(self):
        """Resets the widget’s value to the default"""
        self.setter(self.default)

    def save(self):
        """Saves the widget’s value to Kate’s config"""
        self.conf[self.key] = self.getter()


_JR_DEFAULT = """<script type="text/javascript">
\t(function($) {
\t\t$(document).ready(function() {
\t\t\t$("%s").click(function() {
\t\t\t\t// Write here
\t\t\t});
\t\t});
\t})(jQuery);
</script>
""" % text.TEXT_TO_CHANGE

SETTING_INDENT_JSON         = Setting('JSUtils:indentJSON',            4)
SETTING_ENCODING_JSON       = Setting('JSUtils:encodingJSON',    'utf-8')
SETTING_LINTER              = Setting('JSUtils:linter',                0, choices=['JSHint', 'JSLint'])
SETTING_LINT_ON_SAVE        = Setting('JSUtils:jslintCheckSave',    True)
SETTING_JS_AUTOCOMPLETE     = Setting('JSUtils:jsAutoComplete',     True)
SETTING_JQUERY_AUTOCOMPLETE = Setting('JSUtils:JQueryAutoComplete', True)
SETTING_JQUERY_READY        = Setting('JSUtils:textJQuery',  _JR_DEFAULT, area=True)


# kate: space-indent on; indent-width 4;
