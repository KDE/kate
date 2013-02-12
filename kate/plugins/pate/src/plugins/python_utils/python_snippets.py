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
from python_settings import (KATE_ACTIONS, PYTHON_SPACES,
                             TEXT_INIT, TEXT_SUPER,
                             TEXT_RECURSIVE_CLASS,
                             TEXT_RECURSIVE_NO_CLASS)

str_blank = "(?:\ |\t|\n)*"
str_espaces = "([\ |\t|\n]*)"
pattern_espaces = re.compile("%s(.*)" % str_espaces)
pattern_class = re.compile("%(str_blank)sclass %(str_blank)s(\w+)\(([\w|., ]*)\):" % {'str_blank': str_blank})

str_params = "(.*)"
str_def_init = "%(espaces)sdef %(blank)s(\w+)%(blank)s\(%(param)s" % {
                                                "blank": str_blank,
                                                "espaces": str_espaces,
                                                "param": str_params}
str_def_finish = "(?:.*)\):"
pattern_def_init = re.compile(str_def_init, re.MULTILINE | re.DOTALL)
pattern_def_finish = re.compile(str_def_finish, re.MULTILINE | re.DOTALL)
pattern_def = re.compile("%s\):" % str_def_init, re.MULTILINE | re.DOTALL)

pattern_param = re.compile("%(espaces)s(\w+)%(blank)s\=%(blank)s(.*)" % {
                                                "blank": str_blank,
                                                "espaces": str_espaces},
                                                re.MULTILINE | re.DOTALL)



@kate.action(**KATE_ACTIONS['insertIPDB'])
def insertIPDB():
    """Insert the instructions to debug the python code"""
    insertText("import ipdb; ipdb.set_trace()")


@kate.action(**KATE_ACTIONS['insertInit'])
def insertInit():
    """Insert the __init__ function (Into a class)"""
    class_name = TEXT_TO_CHANGE
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    currentPosition = view.cursorPosition()
    currentLine = currentPosition.line()
    while currentLine >= 0:
        text = unicode(currentDocument.line(currentLine))
        match = pattern_class.match(text)
        if match:
            class_name = match.groups()[0]
            break
        currentLine = currentLine - 1
    insertText(TEXT_INIT % class_name)


def change_kwargs(param):
    match = pattern_param.match(param)
    if match:
        return '%s%s=%s' % (match.groups()[0],
                            match.groups()[1],
                            match.groups()[1])
    return param


def get_number_espaces(currentDocument, currentLine,
                       parentheses=0, initial_instruct=False):
    line_before = unicode(currentDocument.line(currentLine - 1))
    if not line_before.strip() and currentLine >= 0:
        return get_number_espaces(currentDocument, currentLine - 1,
                                  parentheses=parentheses,
                                  initial_instruct=initial_instruct)
    match = pattern_espaces.match(line_before)
    if match:
        if line_before.endswith(":") or parentheses > 0 or initial_instruct:
            parentheses += line_before.count(")") - line_before.count("(")
            line_before = line_before.strip()
            if parentheses == 0 and (line_before.startswith("for") or
                                     line_before.startswith("if") or
                                     line_before.startswith("while") or
                                     line_before.startswith("def")):
                return PYTHON_SPACES + len(match.groups()[0])
            return get_number_espaces(currentDocument, currentLine - 1,
                                      parentheses=parentheses,
                                      initial_instruct=True)
        return len(match.groups()[0])
    return PYTHON_SPACES * 2


def get_prototype_of_current_func():
    espaces = ' ' * PYTHON_SPACES
    number_espaces = PYTHON_SPACES * 2
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
    func_def_espaces = None
    while currentLine >= 0:
        text = unicode(currentDocument.line(currentLine))
        if find_finish_def:
            text_def = '%s\n%s' % (text, text_def)
        else:
            text_def = text
        if function_name == TEXT_TO_CHANGE:
            match_finish = pattern_def_finish.match(text_def)
            match_init = pattern_def_init.match(text_def)
            if match_finish and match_init:
                match = pattern_def.match(text_def)
                number_espaces = get_number_espaces(currentDocument,
                                                    currentPosition.line())
                if not number_espaces:
                    number_espaces = len(match.groups()[0]) + 1
                func_def_espaces = match.groups()[0]
                espaces = ' ' * number_espaces
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
            if func_def_espaces:
                current_spaces = len(text) - len(text.lstrip())
                if current_spaces < func_def_espaces:
                    class_name = match.groups()[0]
                    break
        currentLine = currentLine - 1
    return (espaces, class_name, function_name, params)


@kate.action(**KATE_ACTIONS['insertSuper'])
def insertSuper():
    """Insert the call to the parent method (Into a method of a class)"""
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    currentPosition = view.cursorPosition()
    currentLine = currentPosition.line()
    espaces, class_name, func_name, params = get_prototype_of_current_func()
    text = unicode(currentDocument.line(currentLine)).strip()
    text_super_template = TEXT_SUPER
    if not text:
        currentDocument.removeLine(currentPosition.line())
    else:
        espaces = ''
    if currentLine == currentDocument.lines():
        text_super_template = '\n%s' % text_super_template
    insertText(text_super_template % (espaces, class_name, params[0],
                                      func_name, ', '.join(params[1:])))


@kate.action(**KATE_ACTIONS['callRecursive'])
def callRecursive():
    """Insert the recursive call (Into a method of a class or into a function)"""
    currentDocument = kate.activeDocument()
    view = currentDocument.activeView()
    currentPosition = view.cursorPosition()
    currentLine = currentPosition.line()
    espaces, class_name, func_name, params = get_prototype_of_current_func()
    text = unicode(currentDocument.line(currentLine)).strip()
    if class_name != TEXT_TO_CHANGE:
        text_recursive_template = TEXT_RECURSIVE_CLASS % (espaces, params[0], func_name, ', '.join(params[1:]))
    else:
        text_recursive_template = TEXT_RECURSIVE_NO_CLASS % (espaces, func_name, ', '.join(params))
    if not text:
        currentDocument.removeLine(currentPosition.line())
    else:
        text_recursive_template = text_recursive_template.lstrip()
    if currentLine == currentDocument.lines():
        text_recursive_template = '\n%s' % text_recursive_template
    insertText(text_recursive_template)
