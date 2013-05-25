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

# This file originally was in this repository:
# <https://github.com/goinnn/Kate-plugins/tree/master/kate_plugins/pyte_plugins/text_plugins.py>

import kate
import re

from libkatepate.text import insertText, TEXT_TO_CHANGE
from python_settings import (KATE_ACTIONS,
                             TEXT_INIT, TEXT_SUPER,
                             TEXT_RECURSIVE_CLASS,
                             TEXT_RECURSIVE_NO_CLASS,
                             _IPDB_SNIPPET,
                             DEFAULT_IPDB_SNIPPET)

str_blank = "(?:\ |\t|\n)*"
str_indent = "([\ |\t]*)"
pattern_indent = re.compile("%s(.*)" % str_indent)
pattern_class = re.compile("%(str_indent)sclass %(str_blank)s(\w+)\(([\w|., ]*)\):" % {'str_blank': str_blank,
                                                                                       'str_indent': str_indent})

str_params = "(.*)"
str_def_init = "%(indent)sdef %(blank)s(\w+)%(blank)s\(%(param)s" % {
               "blank": str_blank,
               "indent": str_indent,
               "param": str_params}
str_def_finish = "(?:.*)\):"
pattern_def_init = re.compile(str_def_init, re.MULTILINE | re.DOTALL)
pattern_def_finish = re.compile(str_def_finish, re.MULTILINE | re.DOTALL)
pattern_def = re.compile("%s\):" % str_def_init, re.MULTILINE | re.DOTALL)

pattern_param = re.compile("%(indent)s(\w+)%(blank)s\=%(blank)s(.*)" % {
                           "blank": str_blank,
                           "indent": str_indent},
                           re.MULTILINE | re.DOTALL)


@kate.action(**KATE_ACTIONS['insertIPDB'])
def insertIPDB():
    """Insert the instructions to debug the python code"""
    python_utils_conf = kate.configuration.root.get('python_utils', {})
    ipdb_snippet = python_utils_conf.get(_IPDB_SNIPPET, DEFAULT_IPDB_SNIPPET)
    insertText(ipdb_snippet)


@kate.action(**KATE_ACTIONS['insertInit'])
def insertInit():
    """Insert the __init__ function (Into a class)"""
    indent = ''
    class_name = TEXT_TO_CHANGE
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    currentPosition = view.cursorPosition()
    currentLine = currentPosition.line()
    while currentLine >= 0:
        text = currentDocument.line(currentLine)
        match = pattern_class.match(text)
        if match:
            indent = match.groups()[0]
            class_name = match.groups()[1]
            break
        currentLine = currentLine - 1
    text_init = TEXT_INIT % class_name
    if indent:
        text_init = '\n'.join(['%s%s' % (indent, l) for l in text_init.split('\n')])
    insertText('\n%s' % text_init)


def change_kwargs(param):
    match = pattern_param.match(param)
    if match:
        return '%s%s=%s' % (match.groups()[0],
                            match.groups()[1],
                            match.groups()[1])
    return param


def get_indent(currentDocument, currentLine,
               parentheses=0, initial_instruct=False):
    line_before = currentDocument.line(currentLine - 1)
    if not line_before.strip() and currentLine >= 0:
        return get_indent(currentDocument, currentLine - 1,
                          parentheses=parentheses,
                          initial_instruct=initial_instruct)
    match = pattern_indent.match(line_before)
    if match:
        if line_before.endswith(":") or parentheses > 0 or initial_instruct:
            parentheses += line_before.count(")") - line_before.count("(")
            line_before = line_before.strip()
            if parentheses == 0 and line_before.startswith(("for", "if", "while", "def", "with")):
                return match.groups()[0] + '\t'
            return get_indent(currentDocument, currentLine - 1,
                              parentheses=parentheses,
                              initial_instruct=True)
        return match.groups()[0]
    return '\t\t'


def get_prototype_of_current_func():
    indent = '\t'
    parentheses = 0
    class_name = TEXT_TO_CHANGE
    function_name = TEXT_TO_CHANGE
    params = ['self', '*args', '**kwargs']
    text_def = ''
    find_finish_def = False
    view = kate.activeView()
    currentDocument = kate.activeDocument()
    currentPosition = view.cursorPosition()
    currentLine = currentPosition.line()
    func_def_indent = None
    while currentLine >= 0:
        text = currentDocument.line(currentLine)
        if find_finish_def:
            text_def = '%s\n%s' % (text, text_def)
        else:
            text_def = text
        if function_name == TEXT_TO_CHANGE:
            match_finish = pattern_def_finish.match(text_def)
            match_init = pattern_def_init.match(text_def)
            if match_finish and match_init:
                match = pattern_def.match(text_def)
                indent = get_indent(currentDocument,
                                    currentPosition.line())
                if not indent:
                    indent = match.groups()[0] + '\t'
                func_def_indent = match.groups()[0]
                function_name = match.groups()[1]
                params = match.groups()[2].split(',')
                params = [change_kwargs(param.strip(' \t'))
                          for param in params]
            elif match_finish:
                find_finish_def = True
                parentheses += text_def.count(")") - text_def.count("(")
            if find_finish_def and parentheses <= 0:
                parentheses += text_def.count(")") - text_def.count("(")
                find_finish_def = False
        match = pattern_class.match(text)
        if match:
            if func_def_indent:
                current_indent = len(text) - len(text.lstrip())
                if current_indent < len(func_def_indent):
                    class_name = match.groups()[1]
                    break
        currentLine = currentLine - 1
    return (indent, class_name, function_name, params)


@kate.action(**KATE_ACTIONS['insertSuper'])
def insertSuper():
    """Insert the call to the parent method (Into a method of a class)"""
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    currentPosition = view.cursorPosition()
    currentLine = currentPosition.line()
    indent, class_name, func_name, params = get_prototype_of_current_func()
    text = currentDocument.line(currentLine).strip()
    text_super_template = TEXT_SUPER
    if not text:
        currentDocument.removeLine(currentPosition.line())
    else:
        indent = ''
    if currentLine == currentDocument.lines():
        text_super_template = '\n%s' % text_super_template
    insertText(text_super_template % (indent, class_name, params[0],
                                      func_name, ', '.join(params[1:])))


@kate.action(**KATE_ACTIONS['callRecursive'])
def callRecursive():
    """Insert the recursive call (Into a method of a class or into a function)"""
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    currentPosition = view.cursorPosition()
    currentLine = currentPosition.line()
    indent, class_name, func_name, params = get_prototype_of_current_func()
    text = currentDocument.line(currentLine).strip()
    if class_name != TEXT_TO_CHANGE:
        text_recursive_template = TEXT_RECURSIVE_CLASS % (indent, params[0], func_name, ', '.join(params[1:]))
    else:
        text_recursive_template = TEXT_RECURSIVE_NO_CLASS % (indent, func_name, ', '.join(params))
    if not text:
        currentDocument.removeLine(currentPosition.line())
    else:
        text_recursive_template = text_recursive_template.lstrip()
    if currentLine == currentDocument.lines():
        text_recursive_template = '\n%s' % text_recursive_template
    insertText(text_recursive_template)


# kate: space-indent on; indent-width 4;
