# -*- coding: utf-8 -*-
# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
# Copyright (C) 2013 Shaheed Haque <srhaque@theiet.org>
# Copyright (C) 2013 Alex Turbov <i.zaufi@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) version 3.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

'''Provide shortcuts to access kate internals from plugins'''

import contextlib
import os
import sys

from PyKDE4 import kdecore
from PyKate4.kate import Kate

import pate

class NoActiveView(Exception):
    pass

application = Kate.application()
'''Global accessor to the application object. Equivalent to Kate::application().

Returns: application object.
'''

documentManager = application.documentManager()
'''Global accessor to the document manager object. Equivalent to Kate::documentManager().

Returns: document manager object.
'''

def mainInterfaceWindow():
    ''' The interface to the main window currently showing. Calling
    window() on the interface window gives you the actual
    QWidget-derived main window, which is what the mainWindow()
    function returns '''
    return application.activeMainWindow()


def mainWindow():
    ''' The QWidget-derived main Kate window currently showing. A
    shortcut around kate.application.activeMainWindow().window().

    The Kate API differentiates between the interface main window and
    the actual widget main window. If you need to access the
    Kate.MainWindow for the methods it provides (e.g createToolView),
    then use the mainInterfaceWindow function '''
    assert('Sanity check' and application.activeMainWindow() is not None)
    return application.activeMainWindow().window()


def activeView():
    ''' The currently active view. Access its KTextEditor.Document
    by calling document() on it (or by using kate.activeDocument()).
    This is a shortcut for kate.application.activeMainWindow().activeView()'''
    assert('Sanity check' and application.activeMainWindow() is not None)
    return application.activeMainWindow().activeView()


def activeDocument():
    ''' The document for the currently active view.
    Throws NoActiveView if no active view available.'''
    view = activeView()
    if view:
        return view.document()
    raise NoActiveView


def centralWidget():
    ''' The central widget that holds the tab bar and the editor.
    This is a shortcut for kate.application.activeMainWindow().centralWidget() '''
    return application.activeMainWindow().centralWidget()


def applicationDirectories(*path):
    path = os.path.join('pate', *path)
    return kdecore.KGlobal.dirs().findDirs('appdata', path)


def findApplicationResource(*path):
    path = os.path.join('pate', *path)
    return kdecore.KGlobal.dirs().findResource('appdata', path)


def objectIsAlive(obj):
    ''' Test whether an object is alive; that is, whether the pointer
    to the object still exists. '''
    import sip
    try:
       sip.unwrapinstance(obj)
    except RuntimeError:
       return False
    return True


def qDebug(text):
    '''Use KDE way to show debug info

        TODO Add a way to control debug output from partucular plugins (?)
    '''
    plugin = sys._getframe(1).f_globals['__name__']
    pate.qDebug('{}: {}'.format(plugin, text))


@contextlib.contextmanager
def makeAtomicUndo(document):
    ''' Context manager to make sure startEditing synchronized w/
        endEditing for particular document.
    '''
    document.startEditing()
    try:
        yield
    finally:
        document.endEditing()
