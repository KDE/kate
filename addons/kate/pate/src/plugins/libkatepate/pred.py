#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2010-2013 by Alex Turbov <i.zaufi@gmail.com>
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

''' Reusable code for Kate/Pâté plugins: predicates for text processing '''

def blockCommentStart(line):
    ''' True if line contains C/C++ block-comment-start '''
    return '/*' in line


def blockCommentEnd(line):
    ''' True if line contains C/C++ block-comment-end '''
    return '*/' in line


def startsWith(start):
    ''' True if passed line starts with a given string '''
    def wrapper(line):
        return line.lstrip().startswith(start)
    return wrapper


def equalTo(text):
    ''' True if passed line equals to a given string '''
    def wrapper(line):
        return line.strip() == text
    return wrapper


def onlySingleLineComment(line):
    ''' True if line contains only single line comment '''
    return line.lstrip().startswith('//')


def neg(predicate):
    ''' Negate given predicate '''
    def wrp(*args, **kwargs):
        return not predicate(*args, **kwargs)
    return wrp


def any_of(*predicates):
    ''' Evaluate given predicates list 'till one of 'em is True '''
    def wrp(*args, **kwargs):
        for predicate in predicates:
            if predicate(*args, **kwargs):
                return True
        return False
    return wrp


def all_of(*predicates):
    ''' Evaluate given predicates list 'till one of 'em is False '''
    def wrp(*args, **kwargs):
        for predicate in predicates:
            if not predicate(*args, **kwargs):
                return False
        return True
    return wrp
