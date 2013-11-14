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
import sys
import traceback
import xml.dom.minidom


from PyQt4 import QtCore, QtGui
from PyKDE4 import kdecore, kdeui

import pate

from .api import *

_registered_xml_gui_clients = dict()

#
# Initialize related stuff
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
        event.functions[plugin] = []

    event.functions[plugin].append(func)
    return func


@_simpleEventListener
def init(func):
    ''' The function will be called when particular plugin has loaded
        and the configuration has been initiated
    '''
    plugin = sys._getframe(1).f_globals['__name__']
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
    def xml_gui_remover():
        if plugin in _registered_xml_gui_clients and mainInterfaceWindow() is not None:
            clnt = _registered_xml_gui_clients[plugin]
            clnt.actionCollection().clearAssociatedWidgets()
            clnt.actionCollection().clear()
            mainInterfaceWindow().guiFactory().removeClient(clnt)
            del _registered_xml_gui_clients [plugin]
        func()

    return _registerCallback(plugin, unload, xml_gui_remover)


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
    print('---------@action: {}/{}'.format(plugin,func.__name__))

    ui_file = kdecore.KGlobal.dirs().findResource('appdata', 'pate/{}_ui.rc'.format(plugin))
    print('ui_file={}'.format(repr(ui_file)))
    clnt = kdeui.KXMLGUIClient()
    clnt.replaceXMLFile(ui_file,ui_file)
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
            # Get optional attributes
            if 'shortcut' in tag.attributes:
                act.setShortcut(QtGui.QKeySequence(tag.attributes['shortcut']))
            if 'icon' in tag.attributes:
                act.setIcon(kdeui.KIcon(tag.attributes['icon']))
            if 'iconText' in tag.attributes:
                act.setIconText(tag.attributes['iconText'])
            if 'toolTip' in tag.attributes:
                act.setToolTip(tag.attributes['toolTip'])
            if 'whatsThis' in tag.attributes:
                act.setWhatsThis(tag.attributes['whatsThis'])
            # Connect it to the function
            act.connect(act, QtCore.SIGNAL('triggered()'), func)

    if not found:
        # TODO Show some SPAM about inconsistent action
        pass


    mainInterfaceWindow().guiFactory().addClient(clnt)
    _registered_xml_gui_clients[plugin] = clnt

    # TODO Provide an access to the XML GUI client to the plugin
    # so it may control actions ... from their actions ;-)

    return func

# kate: space-indent on; indent-width 4;
