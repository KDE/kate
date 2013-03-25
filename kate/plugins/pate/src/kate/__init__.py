# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
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

"""Namespace for Kate application interfaces.

This namespace contains interfaces for plugin developers for the Kate application.
"""

from __future__ import print_function
import sys
import os
import traceback
import functools
import pydoc
from inspect import getmembers, isfunction
from PyKDE4.kdecore import i18n
from PyKDE4.kdeui import KMessageBox

import pate
import kate.gui

from PyQt4 import QtCore, QtGui
from PyKDE4 import kdecore, kdeui
# kate namespace
from PyKate4.kate import Kate
from PyKDE4.ktexteditor import KTextEditor

# Plugin API

# Configuration

class Configuration:
    '''Configuration objects provide plugin-specific configuration dictionary.

    In other words, each plugin uses kate.configuration and the class
    automatically creates a plugin-specific dictionary to support it.

    The config is saved and loaded from disk automatically for minimal user
    hassle. Just go ahead and use kate.configuration as a persistent dictionary.
    Do not instantiate your own Configuration object; use kate.configuration
    instead.

    Use a string key. Any Python type that can be pickled is can be used as a
    value -- dictionaries, lists, numbers, strings, sets, and so on.
    '''

    def __init__(self, root):
        self.root = root

    def __getitem__(self, key):
        plugin = sys._getframe(1).f_globals['__name__']
        return self.root.get(plugin, {})[key]

    def __setitem__(self, key, value):
        plugin = sys._getframe(1).f_globals['__name__']
        if plugin not in self.root:
            self.root[plugin] = {}
        self.root[plugin][key] = value

    def __delitem__(self, key):
        plugin = sys._getframe(1).f_globals['__name__']
        del self.root.get(plugin, {})[key]

    def __contains__(self, key):
        plugin = sys._getframe(1).f_globals['__name__']
        return key in self.root.get(plugin, {})

    def __len__(self):
        plugin = sys._getframe(1).f_globals['__name__']
        return len(self.root.get(plugin, {}))

    def __iter__(self):
        plugin = sys._getframe(1).f_globals['__name__']
        return iter(self.root.get(plugin, {}))

    def __str__(self):
        plugin = sys._getframe(1).f_globals['__name__']
        return str(self.root.get(plugin, {}))

    def __repr__(self):
        plugin = sys._getframe(1).f_globals['__name__']
        return repr(self.root.get(plugin, {}))

    def keys(self):
        plugin = sys._getframe(1).f_globals['__name__']
        return self.root.get(plugin, {}).keys()

    def values(self):
        plugin = sys._getframe(1).f_globals['__name__']
        return self.root.get(plugin, {}).values()

    def items(self):
        plugin = sys._getframe(1).f_globals['__name__']
        return self.root.get(plugin, {}).items()

    def get(self, key, default=None):
        plugin = sys._getframe(1).f_globals['__name__']
        try:
            return self[plugin][key]
        except KeyError:
            return default

    def pop(self, key):
        value = self[key]
        del self[key]
        return value

    def save(self):
        pate.saveConfiguration()

    def _name(self):
        return sys._getframe(1).f_globals['__name__']

globalConfiguration = pate.configuration
"""Configuration for all plugins.

This can also be used by one plugin to access another plugin's configurations.
"""

configuration = Configuration(pate.configuration)
"""Configuration for this plugin."""

def sessionConfiguration():
    """Deprecated. Use globalConfiguration instead."""
    if plugins is not None:
        return pate.configuration

def plugins():
    """ The list of available plugins."""
    return pate.plugins

def pluginDirectories():
    """ The list of plugin directories."""
    return pate.pluginDirectories

def moduleGetHelp(module):
    """Fetch HTML documentation on some module."""
    return pydoc.html.page(pydoc.describe(module), pydoc.html.document(module, module.__name__))

def _moduleActionDecompile(action):
    """Deconstruct an @action."""
    if len(action.text()) > 0:
        text = action.text()
    else:
        text = None

    if len(action.icon().name()) > 0:
        icon = action.icon().name()
    elif action.icon().isNull():
        icon = None
    else:
        icon = action.icon()

    if 'menu' in action.__dict__:
        menu = action.__dict__['menu']
    else:
        menu = None

    if len(action.shortcut().toString()) > 0:
        shortcut = action.shortcut().toString()
    else:
        shortcut = None
    return (text, icon, shortcut, menu)

def moduleGetActions(module):
    """Return a list of each module function decorated with @action.

    The returned object is [ { function, ( text, icon, shortcut, menu), help }... ].
    """
    try:
        result = []
        for k, v in module.__dict__.items():
            if isinstance(v, kate.catchAllHandler):
                result.append((k, _moduleActionDecompile(v.action), getattr(v.f, "__doc__", None)))
        return result
    except IndexError:
        # Python3 throws this for modules, though Python 2 does not.
        sys.stderr.write("IndexError getting actions for " + str(module) + "\n")
        return []

def moduleGetConfigPages(module):
    """Return a list of each module function decorated with @configPage.

    The returned object is [ { function, callable, ( name, fullName, icon ) }... ].
    """
    result = []
    for k, v in module.__dict__.items():
        if isfunction(v) and hasattr(v, "configPage"):
            result.append((k, v, v.configPage))
    return result

def _callAll(l, *args, **kwargs):
    for f in l:
        try:
            f(*args, **kwargs)
        except:
            traceback.print_exc()
            sys.stderr.write('\n')
            continue

def _attribute(**attributes):
    # utility decorator that we wrap events in. Simply initialises
    # attributes on the function object to make code nicer.
    def decorator(func):
        for key, value in attributes.items():
            setattr(func, key, value)
        return func
    return decorator

def _simpleEventListener(func):
    # automates the most common decorator pattern: calling a bunch
    # of functions when an event has occured
    func.functions = set()
    func.fire = functools.partial(_callAll, func.functions)
    func.clear = func.functions.clear
    return func

# Decorator event listeners

@_simpleEventListener
def init(func):
    ''' The function will be called when Kate has loaded completely: when all
    other enabled plugins have been loaded into memory, the configuration has
    been initiated, etc. '''
    init.functions.add(func)
    return func

@_simpleEventListener
def unload(func):
    ''' The function will be called when Pate is being unloaded from memory.
    Clean up any widgets that you have added to the interface (toolviews
    etc). '''
    unload.functions.add(func)
    return func

@_simpleEventListener
def viewChanged(func):
    ''' Calls the function when the view changes. To access the new active view,
    use kate.activeView() '''
    viewChanged.functions.add(func)
    return func

@_simpleEventListener
def viewCreated(func):
    ''' Calls the function when a new view is created, passing the view as a
    parameter '''
    viewCreated.functions.add(func)
    return func

class _HandledException(Exception):
    def __init__(self, message):
        super(_HandledException, self).__init__(message)

class catchAllHandler(object):
    '''Standard error handling for plugin actions.'''
    def __init__(self, f):
        self.f = f

    def __call__(self):
        try:
            return self.f()
        except _HandledException:
            raise
        except Exception as e:
            txt = "".join(traceback.format_exception(*sys.exc_info()))
            # TODO: Use the i18n() wrapper here. Unfortunately, Python3 currently
            # says "TypeError: i18n(): argument 1 has unexpected type 'str'",
            # and that is rather unfortunate for the catch-all handler.
            KMessageBox.error(None, txt, "Error in action '{}'".format(self.f.__name__))
            raise _HandledException(txt)

@_attribute(actions=set())
def action(text, icon=None, shortcut=None, menu=None):
    ''' Decorator that adds an action to the menu bar. When the item is fired,
    your function is called. Optional shortcuts, menu to place the action in,
    and icon can be specified.

    Parameters:
        * text -        The text associated with the action (used as the menu
                        item label, etc).
        * shortcut -    The shortcut to fire this action or None if there is no
                        shortcut. Must be a string such as 'Ctrl+1' or a
                        QKeySequence instance. By default no shortcut is set (by
                        passing None)
        * icon -        An icon to associate with this action. It is shown
                        alongside text in the menu bar and in toolbars as
                        required. Pass a string to use KDE's image loading
                        system or a QPixmap or QIcon to use any custom icon.
                        None (the default) sets no icon.
        * menu -        The menu under which to place this item. Must be a
                        string such as 'tools' or 'settings', or None to not
                        place it in any menu.

    The docstring for your action will be used to provide "help" text.

    NOTE: Kate may need to be restarted for this decorator to take effect, or
    to remove all traces of the plugin on removal.
    '''
    def decorator(func):
        a = kdeui.KAction(text, None)
        if shortcut is not None:
            if isinstance(shortcut, str):
                a.setShortcut(QtGui.QKeySequence(shortcut))
            else:
                a.setShortcut(shortcut)
        if icon is not None:
            a.setIcon(kdeui.KIcon(icon))
        a.menu = menu
        func = catchAllHandler(func)
        a.connect(a, QtCore.SIGNAL('triggered()'), func)
        # delay till everything has been initialised
        action.actions.add(a)
        func.action = a
        return func
    return decorator

@_attribute(actions=set())
def configPage(name, fullName, icon):
    ''' Decorator that adds a configPage into Kate's settings dialog. When the
    item is fired, your function is called.

    Parameters:
        * name -        The text associated with the configPage in the list of
                        config pages.
        * fullName -    The title of the configPage when selected.
        * icon -        An icon to associate with this configPage. Pass a string
                        to use KDE's image loading system or a QPixmap or
                        QIcon to use any custom icon.

    NOTE: Kate may need to be restarted for this decorator to take effect, or
    to remove all traces of the plugin on removal.
    '''
    def decorator(func):
        a = name, fullName, kdeui.KIcon(icon)
        configPage.actions.add(a)
        func.configPage = a
        return func
    return decorator

# End decorators


# API functions and objects

class NoActiveView(Exception):
    pass

application = Kate.application()
"""Global accessor to the application object. Equivalent to Kate::application().

Returns: application object.
"""

documentManager = application.documentManager()
"""Global accessor to the document manager object. Equivalent to Kate::documentManager().

Returns: document manager object.
"""

def mainWindow():
    ''' The QWidget-derived main Kate window currently showing. A
    shortcut around kate.application.activeMainWindow().window().

    The Kate API differentiates between the interface main window and
    the actual widget main window. If you need to access the
    Kate.MainWindow for the methods it provides (e.g createToolView),
    then use the mainInterfaceWindow function '''
    return application.activeMainWindow().window()

def mainInterfaceWindow():
    ''' The interface to the main window currently showing. Calling
    window() on the interface window gives you the actual
    QWidget-derived main window, which is what the mainWindow()
    function returns '''
    return application.activeMainWindow()

def activeView():
    ''' The currently active view. Access its KTextEditor.Document
    by calling document() on it (or by using kate.activeDocument()).
    This is a shortcut for kate.application.activeMainWindow().activeView()'''
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

def focusEditor():
    ''' Give the editing section focus '''
    print('dumping tree....')
    for x in mainWindow().findChildren(QtGui.QWidget):
        print(x.__class__.__name__, x.objectName())

def applicationDirectories(*path):
    path = os.path.join('pate', *path)
    return kdecore.KGlobal.dirs().findDirs("appdata", path)

def objectIsAlive(obj):
    ''' Test whether an object is alive; that is, whether the pointer
    to the object still exists. '''
    import sip
    try:
       sip.unwrapinstance(obj)
    except RuntimeError:
       return False
    return True


# Initialisation

def pateInit():
    # wait for the configuration to be read
    def _initPhase2():
        global initialized
        initialized = True
        # set up actions -- plug them into the window's action collection
        windowInterface = application.activeMainWindow()
        window = windowInterface.window()
        nameToMenu = {} # e.g "help": KMenu
        for menu in window.findChildren(QtGui.QMenu):
            name = str(menu.objectName())
            if name:
                nameToMenu[name] = menu
        collection = window.actionCollection()
        for a in action.actions:
            # allow a configurable name so that built-in actions can be
            # overriden?
            collection.addAction(a.text(), a)
            if a.menu is not None:
                # '&Blah' => 'blah'
                menuName = a.menu.lower().replace('&', '')
                if menuName not in nameToMenu:
                    #
                    # Plugin wants to create an item in a.menu which does not
                    # exist, so we need to create it.
                    #
                    before = nameToMenu['help'].menuAction()
                    menu = kdeui.KMenu(a.menu, window.menuBar())
                    menu.setObjectName(menuName)
                    window.menuBar().insertMenu(before, menu)
                    nameToMenu[menuName] = menu
                nameToMenu[menuName].addAction(a)
        # print 'init:', Kate.application(), application.activeMainWindow()
        windowInterface.connect(windowInterface, QtCore.SIGNAL('viewChanged()'), viewChanged.fire)
        windowInterface.connect(windowInterface, QtCore.SIGNAL('viewCreated(KTextEditor::View*)'), viewCreated.fire)
        _callAll(init.functions)
    QtCore.QTimer.singleShot(0, _initPhase2)

# called by pate on initialisation
pate._pluginsLoaded = pateInit
del pateInit


def pateDie():
    # Unload actions or things will crash
    for a in action.actions:
        for w in a.associatedWidgets():
            w.removeAction(a)
    # clear up
    unload.fire()

    action.actions.clear()
    init.clear()
    unload.clear()
    viewChanged.clear()
    viewCreated.clear()

pate._pluginsUnloaded = pateDie
del pateDie

def pateSessionInit():
    pass
    # print 'new session:', Kate.application(), application.activeMainWindow()

pate._sessionCreated = pateSessionInit
del pateSessionInit

kate.gui.loadIcon = lambda s: kdeui.KIcon(s).pixmap(32, 32)

# kate: space-indent on; indent-width 4;
