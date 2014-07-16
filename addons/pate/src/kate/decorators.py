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

'''Decorators used in plugins'''

import functools
import inspect
import sys
import traceback
import xml.dom.minidom

from PyQt4 import QtCore, QtGui
from PyKDE4 import kdecore, kdeui
from PyKDE4.kdecore import i18nc

import pate

from .api import *


_registered_xml_gui_clients = dict()


def getXmlGuiClient(plugin=None, ef=1, use_inspect=False):
    '''Provide an access to an XML GUI client for a current plugin'''
    if plugin is None:
        if use_inspect:
            plugin = inspect.getmoduleinfo(inspect.stack()[ef + 1][1])[0]
        else:
            plugin = sys._getframe(1).f_globals['__name__']

    qDebug('Getting XMLGUIClient for {}/{}/{}'.format(plugin, ef, use_inspect))
    if plugin in _registered_xml_gui_clients:
        return _registered_xml_gui_clients[plugin]


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
                # TODO Return smth to a caller, so in case of
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
    qDebug('@init: {}/{}'.format(plugin, func.__name__))
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
    qDebug('@unload: {}/{}'.format(plugin, func.__name__))
    def _module_cleaner():
        qDebug('@unload/cleaner: {}/{}'.format(plugin, func.__name__))
        if plugin in _registered_xml_gui_clients and mainInterfaceWindow() is not None:
            clnt = _registered_xml_gui_clients[plugin]
            clnt.actionCollection().clearAssociatedWidgets()
            clnt.actionCollection().clear()
            mainInterfaceWindow().guiFactory().removeClient(clnt)
            del _registered_xml_gui_clients[plugin]

        if plugin in init.functions:
            qDebug('@unload/init-cleaner: {}/{}'.format(plugin, func.__name__))
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
              , xi18nc('@title:window', 'Error in action <icode>%1</icode>', self.f.__name__)
              )
            raise _HandledException(txt)


class InvalidAction(Exception):
    pass


def action(func):
    ''' Decorator that adds an action to the menu bar. When the item is fired,
        your function is called
    '''
    frame = sys._getframe(1)
    plugin = frame.f_globals['__name__']
    qDebug('@action: {}/{}'.format(plugin, func.__name__))

    # Get directory where plugin resides
    filename = frame.f_globals['__file__']
    dirname = os.path.dirname(filename)
    pate_pos = dirname.find('pate')
    assert(pate_pos != -1)
    dirname = dirname[pate_pos:]
    filename = os.path.join(dirname, '{}_ui.rc'.format(plugin))
    ui_file = kdecore.KGlobal.dirs().findResource('appdata', filename)
    if not ui_file:
        ui_file = kdecore.KGlobal.dirs().findResource('appdata', 'pate/{}/{}_ui.rc'.format(plugin, plugin))
        if not ui_file:
            # TODO Report an error: found an action w/o corresponding ui.rc file!
            return func

    # Found UI resource file
    qDebug('ui_file={}'.format(repr(ui_file)))

    # Get the XML GUI client or create a new one
    clnt = None
    if plugin not in _registered_xml_gui_clients:
        qDebug('@action: make XMLGUIClient for plugin={}'.format(plugin))
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
        want_it = 'name' in tag.attributes.keys() \
            and 'text' in tag.attributes.keys() \
            and func.__name__ == tag.attributes['name'].value
        if want_it:
            found = True
            # Get mandatory attributes
            name = tag.attributes['name'].value
            text = tag.attributes['text'].value
            act = clnt.actionCollection().addAction(name)
            act.setText(text)
            qDebug('@action/found: {} --> "{}"'.format(name, text))
            # Get optional attributes
            if 'shortcut' in tag.attributes.keys():
                act.setShortcut(QtGui.QKeySequence(tag.attributes['shortcut'].value))
            if 'icon' in tag.attributes.keys():
                act.setIcon(kdeui.KIcon(tag.attributes['icon'].value))
            if 'iconText' in tag.attributes.keys():
                act.setIconText(tag.attributes['iconText'].value)
            if 'toolTip' in tag.attributes.keys():
                act.setToolTip(tag.attributes['toolTip'].value)
            if 'whatsThis' in tag.attributes.keys():
                act.setWhatsThis(tag.attributes['whatsThis'].value)
            # Connect it to the function
            act.connect(act, QtCore.SIGNAL('triggered()'), func)
            # Make an action accessible from the targeted function as its attribute
            setattr(func, 'action', act)
            # No need to scan anything else!!!
            # ATTENTION It is why the very first action declaration
            # MUST contain everything! While others, w/ same name,
            # may have only name! For example, if same action must be
            # added to toolbar or context menu...
            break

    if not found:
        raise InvalidAction(
            xi18nc(
                '@info:tooltip'
              , 'Invalid <filename>%1</filename> file: No action with name <icode>%2</icode> defined'
              , ui_file
              , func.__name__
              )
          )

    if plugin not in _registered_xml_gui_clients:
        _registered_xml_gui_clients[plugin] = clnt

    mainInterfaceWindow().guiFactory().addClient(clnt)

    return func
