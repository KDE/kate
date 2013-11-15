# -*- coding: utf-8 -*-
#
# This file is part of Pate, Kate' Python scripting plugin.
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

'''Decorators used in plugins'''

import functools
import inspect
import sys
import traceback
import xml.dom.minidom


from PyQt4 import QtCore, QtGui
from PyKDE4 import kdecore, kdeui

import pate

from .api import *

_registered_xml_gui_clients = dict()

#
# initialization related stuff
#

def pateEventHandler(event):
    def _decorator(func):
        setattr(pate, event, func)
        del func
    return _decorator


def _callAll(plugin, functions, *args, **kwargs):
    if plugin in functions:
        for f in functions[plugin]:
            try:
                f(*args, **kwargs)
            except:
                traceback.print_exc()
                sys.stderr.write('\n')
                # TODO Return smth to the caller, so in case of
                # failed initialization it may report smth to the
                # C++ level and latter can show an error to the user...
                continue


def _simpleEventListener(func):
    # automates the most common decorator pattern: calling a bunch
    # of functions when an event has occurred
    func.functions = dict()
    func.fire = functools.partial(_callAll, functions=func.functions)
    func.clear = func.functions.clear
    return func


def _registerCallback(plugin, event, func):
    if plugin not in event.functions:
        event.functions[plugin] = set()

    event.functions[plugin].add(func)
    return func


@_simpleEventListener
def init(func):
    ''' The function will be called when particular plugin has loaded
        and the configuration has been initiated
    '''
    plugin = sys._getframe(1).f_globals['__name__']
    print('@init: {}/{}'.format(plugin, func.__name__))
    return _registerCallback(plugin, init, func)


@_simpleEventListener
def unload(func):
    ''' The function will be called when particular plugin is being
        unloaded from memory. Clean up any widgets that you have added
        to the interface (toolviews etc).

        ATTENTION Be really careful trying to access any window, view
            or document from the @unload handler: in case of application
            quit everything is dead already!
    '''
    plugin = sys._getframe(1).f_globals['__name__']
    print('@unload: {}/{}'.format(plugin, func.__name__))
    def _module_cleaner():
        print('@unload/cleaner: {}/{}'.format(plugin, func.__name__))
        if plugin in _registered_xml_gui_clients and mainInterfaceWindow() is not None:
            clnt = _registered_xml_gui_clients[plugin]
            clnt.actionCollection().clearAssociatedWidgets()
            clnt.actionCollection().clear()
            mainInterfaceWindow().guiFactory().removeClient(clnt)
            del _registered_xml_gui_clients[plugin]

        if plugin in init.functions:
            print('@unload/init-cleaner: {}/{}'.format(plugin, func.__name__))
            del init.functions[plugin]

        func()

    return _registerCallback(plugin, unload, _module_cleaner)


@_simpleEventListener
def viewCreated(func):
    ''' Calls the function when a new view is created, passing the
        view as a parameter.
    '''
    plugin = sys._getframe(1).f_globals['__name__']
    mainWindow = application.activeMainWindow()
    assert(mainWindow is not None)
    mainWindow.connect(mainWindow, QtCore.SIGNAL('viewCreated(KTextEditor::View*)'), func)
    return func


@_simpleEventListener
def viewChanged(func):
    ''' Calls the function when the view changes.
        To access the new active view, use kate.activeView()

        NOTE The very first call after kate starts the view actually is None!
    '''
    plugin = sys._getframe(1).f_globals['__name__']
    mainWindow = application.activeMainWindow()
    assert(mainWindow is not None)
    mainWindow.connect(mainWindow, QtCore.SIGNAL('viewChanged()'), func)
    return func


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
    plugin = sys._getframe(1).f_globals['__name__']
    #print('---------@configPage[{}]: name={}, fullName={}, icon={}'.format(plugin, repr(name), fullName, icon))

    def _decorator(func):
        a = name, fullName, kdeui.KIcon(icon)
        func.configPage = a
        return func
    return _decorator


def moduleGetConfigPages(module):
    '''Return a list of each module function decorated with @configPage.

    The returned object is [ { function, callable, ( name, fullName, icon ) }... ].
    '''
    result = []
    for k, v in module.__dict__.items():
        if inspect.isfunction(v) and hasattr(v, 'configPage'):
            result.append((k, v, v.configPage))
    return result

#
# Actions related stuff
#

class _HandledException(Exception):
    def __init__(self, message):
        super(_HandledException, self).__init__(message)


class _catchAllHandler(object):
    '''Standard error handling for plugin actions.'''
    def __init__(self, f):
        self.f = f

    def __call__(self):
        try:
            return self.f()
        except _HandledException:
            raise
        except Exception as e:
            txt = ''.join(traceback.format_exception(*sys.exc_info()))
            KMessageBox.error(
                None
              , txt
              , i18nc('@title:window', 'Error in action <icode>%1</icode>', self.f.__name__)
              )
            raise _HandledException(txt)


def action(func):
    ''' Decorator that adds an action to the menu bar. When the item is fired,
        your function is called
    '''
    plugin = sys._getframe(1).f_globals['__name__']
    print('@action: {}/{}'.format(plugin, func.__name__))
    ui_file = kdecore.KGlobal.dirs().findResource('appdata', 'pate/{}_ui.rc'.format(plugin))
    if not ui_file:
        ui_file = kdecore.KGlobal.dirs().findResource('appdata', 'pate/{}/{}_ui.rc'.format(plugin, plugin))
        if not ui_file:
            return func

    # Found UI resource file
    #print('ui_file={}'.format(repr(ui_file)))

    # Get the XML GUI client or create a new one
    clnt = None
    if plugin not in _registered_xml_gui_clients:
        #print('----@action: make XMLGUICline 4 plugin={}'.format(repr(plugin)))
        clnt = kdeui.KXMLGUIClient()
        clnt.replaceXMLFile(ui_file,ui_file)
    else:
        clnt = _registered_xml_gui_clients[plugin]
        mainInterfaceWindow().guiFactory().removeClient(clnt)

    # Find an action details
    gui = xml.dom.minidom.parse(ui_file)
    found = False
    for tag in gui.getElementsByTagName('Action'):
        # Action at least must have a name and text
        # and name must be the same as a function name marked w/ @action decorator
        want_it = 'name' in tag.attributes \
            and 'text' in tag.attributes \
            and func.__name__ == tag.attributes['name'].value
        if want_it:
            found = True
            # Get mandatory attributes
            name = tag.attributes['name'].value
            text = tag.attributes['text'].value
            act = clnt.actionCollection().addAction(name)
            act.setText(text)
            print('@action/found: {} --> "{}"'.format(name, text))
            # Get optional attributes
            if 'shortcut' in tag.attributes:
                act.setShortcut(QtGui.QKeySequence(tag.attributes['shortcut'].value))
            if 'icon' in tag.attributes:
                act.setIcon(kdeui.KIcon(tag.attributes['icon'].value))
            if 'iconText' in tag.attributes:
                act.setIconText(tag.attributes['iconText'].value)
            if 'toolTip' in tag.attributes:
                act.setToolTip(tag.attributes['toolTip'].value)
            if 'whatsThis' in tag.attributes:
                act.setWhatsThis(tag.attributes['whatsThis'].value)
            # Connect it to the function
            act.connect(act, QtCore.SIGNAL('triggered()'), func)

    if not found:
        # TODO Show some SPAM about inconsistent action
        pass

    if plugin not in _registered_xml_gui_clients:
        _registered_xml_gui_clients[plugin] = clnt

    mainInterfaceWindow().guiFactory().addClient(clnt)

    # TODO Provide an access to the XML GUI client to the plugin
    # so it may control actions ... from their actions ;-)

    return func

# kate: space-indent on; indent-width 4;
