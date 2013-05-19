# -*- coding: utf-8 -*-
"""IPython console for hacking kate and doing science"""

# Uses this: http://stackoverflow.com/a/11525205/247482
#
# May need reworking with IPython 1.0 due to changed event loop


# TODO: fix help() by fixing input()

from __future__ import unicode_literals

import sys

from PyKDE4.kdecore import i18n as _translate
from libkatepate.errors import needs_packages

sys.argv = [__file__]

NEED_PACKAGES = {
    'IPython': 'ipython>=0.13.1',
    'readline': '6.2.4.1',
    'pygments': 'Pygments==1.6',
}


if sys.version_info.major == 3:
    NEED_PACKAGES['zmq'] = 'pyzmq==13.0.0'
    _u = str
else:
    NEED_PACKAGES['zmq'] = 'pyzmq==2.0.10.1'
    _u = unicode

needs_packages(NEED_PACKAGES)


def i18n(msg, *args):
    if isinstance(msg, _u):
        msg = msg.encode('utf-8')
    return _translate(msg, *args) if args else _translate(msg)


import atexit
import kate
import os

try:  # ≥1.0
    from IPython.kernel.inprocess.ipkernel import InProcessKernel
    from IPython.frontend.qt.inprocess import QtInProcessKernelManager
    ipython_1 = True
except ImportError:  # ≤0.13
    from IPython.zmq.ipkernel import IPKernelApp
    from IPython.frontend.qt.kernelmanager import QtKernelManager
    from IPython.lib.kernel import find_connection_file
    ipython_1 = False
from IPython.frontend.qt.console.rich_ipython_widget import RichIPythonWidget

from PyQt4 import uic
from PyQt4.QtGui import QWidget

from libkatepate.project_utils import (
    get_project_plugin,
    is_version_compatible,
    add_extra_path,
    add_environs)

try:  # fix for Kate 4.10
    get_project_plugin()
except AttributeError:
    get_project_plugin = lambda: None

_SCROLLBACK_LINES_COUNT_CFG = 'ipythonConsole:scrollbackLinesCount'
_GUI_COMPLETION_TYPE_CFG = 'ipythonConsole:guiCompletionType'
_CONFIG_UI = 'python_console_ipython.ui'
_CONSOLE_CSS = 'python_console_ipython.css'
_GUI_COMPLETION_CONVERT = ['droplist', None]


def event_loop(kernel):
    kernel.timer = kate.gui.QTimer()
    kernel.timer.timeout.connect(kernel.do_one_iteration)
    kernel.timer.start(1000 * kernel._poll_interval)


def default_kernel_app():
    app = IPKernelApp.instance()
    if not app.config:
        app.initialize(['python', '--pylab=qt'])  # at this point, print() won’t work anymore
    app.kernel.eventloop = event_loop
    return app


def default_manager(kernel_app):
    connection_file = find_connection_file(kernel_app.connection_file)
    manager = QtKernelManager(connection_file=connection_file)
    manager.load_connection_file()
    manager.start_channels()
    return manager


if ipython_1:
    manager = QtInProcessKernelManager()
    manager.start_kernel()
    kernel = manager.kernel
    kernel.gui = 'qt4'
    client = manager.client()
    client.start_channels()
else:
    kernel = default_kernel_app()
    manager = default_manager(kernel)
    kernel.start()


def django_project_filename_changed():
    # Uses this: https://github.com/django-extensions/django-extensions/blob/master/django_extensions/management/shells.py#L7
    from django.db.models.loading import get_models, get_apps
    loaded_models = get_models()  # NOQA

    from django.conf import settings
    imported_objects = {'settings': settings}

    dont_load = getattr(settings, 'SHELL_PLUS_DONT_LOAD', [])
    model_aliases = getattr(settings, 'SHELL_PLUS_MODEL_ALIASES', {})
    imports = []
    for app_mod in get_apps():
        app_models = get_models(app_mod)
        if not app_models:
            continue
        app_name = app_mod.__name__.split('.')[-2]
        if app_name in dont_load:
            continue
        app_aliases = model_aliases.get(app_name, {})
        model_labels = []
        for model in app_models:
            model_name = model.__name__
            try:
                imported_object = getattr(__import__(app_mod.__name__, {}, {}, model_name), model_name)
                if '{}.{}'.format(app_name, model_name) in dont_load:
                    continue
                alias = app_aliases.get(model_name, model_name)
                imported_objects[alias] = imported_object
                if model_name == alias:
                    model_labels.append(model_name)
                else:
                    model_labels.append('{} (as {})'.format(model_name, alias))
            except AttributeError as reason:
                msg = i18n('Failed to import “%1” from “%2” reason: %3', model_name, app_name, reason)
                kernel.shell.write(msg)
                continue
        msg = i18n('From “%1” autoload: %2', app_mod.__name__.split('.')[-2], ', '.join(model_labels))
        imports.append(msg)
    for import_msg in imports:
        kernel.shell.write(import_msg)
    return imported_objects


def projectFileNameChanged(*args, **kwargs):
    projectPlugin = get_project_plugin()
    projectMap = projectPlugin.property('projectMap')
    if 'python' in projectMap:
        projectName = projectPlugin.property('projectName')
        projectMapPython = projectMap['python']
        version = projectMapPython.get('version', None)
        # Check Python version
        if not is_version_compatible(version):
            msg = i18n('Cannot load this project: %1. Python Version incompatible', projectName)
            kernel.shell.write(msg)
            sys.stdout.flush()
            return
        kernel.shell.reset()
        kernel.shell.write(i18n('Load project: %1', projectName))
        extraPath = projectMapPython.get('extraPath', [])
        environs = projectMapPython.get('environs', {})
        # Add Extra path
        add_extra_path(extraPath)
        # Add environs
        add_environs(environs)
        # Special treatment
        if projectMapPython.get('projectType', '').lower() == 'django':
            imported_objects = django_project_filename_changed()
            kernel.shell.user_ns.update(imported_objects)
        # Print details
        sys.stdout.flush()


def terminal_widget(parent=None, **kwargs):
    try:
        gui_completion = _GUI_COMPLETION_CONVERT[kate.configuration[_GUI_COMPLETION_TYPE_CFG]]
    except KeyError:
        gui_completion = 'droplist'
    if gui_completion:
        widget = RichIPythonWidget(parent=parent, gui_completion=gui_completion)
    else:
        widget = RichIPythonWidget(parent=parent)

    widget.kernel_manager = manager

    if ipython_1:  # https://github.com/ipython/ipython/blob/master/examples/inprocess/embedded_qtconsole.py
        widget.kernel_client = client

        def stop():
            client.stop_channels()
            manager.shutdown_kernel()

        widget.exit_requested.connect(stop)
    else:
        widget.exit_requested.connect(manager.cleanup_connection_file)

    # update namespace
    kernel.shell.user_ns.update(kwargs)
    kernel.shell.user_ns['console'] = widget

    msg = i18n('\nAvailable variables are everything from pylab, “%1”, and this console as “console”', '”, “'.join(kwargs.keys()))

    kernel.shell.write(msg)  # TODO: this only gets shown after the user presses return: a way to flush the shell?

    projectPlugin = get_project_plugin()
    if projectPlugin:
        projectPlugin.projectFileNameChanged.connect(projectFileNameChanged)
        projectFileNameChanged()

    sys.stdout.flush()
    return widget


class ConfigWidget(QWidget):
    """Configuration widget for this plugin."""
    #
    # Lines count
    #
    scrollbackLinesCount = None

    def __init__(self, parent=None, name=None):
        super(ConfigWidget, self).__init__(parent)

        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), _CONFIG_UI), self)

        self.reset()

    def apply(self):
        kate.configuration[_SCROLLBACK_LINES_COUNT_CFG] = self.scrollbackLinesCount.value()
        seletecItems = self.guiCompletionType.selectedItems()
        if seletecItems:
            index = self.guiCompletionType.indexFromItem(seletecItems[0])
            kate.configuration[_GUI_COMPLETION_TYPE_CFG] = index.row()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if _SCROLLBACK_LINES_COUNT_CFG in kate.configuration:
            self.scrollbackLinesCount.setValue(kate.configuration[_SCROLLBACK_LINES_COUNT_CFG])
        if _GUI_COMPLETION_TYPE_CFG in kate.configuration and isinstance(kate.configuration[_GUI_COMPLETION_TYPE_CFG], int):
            item = self.guiCompletionType.item(kate.configuration[_GUI_COMPLETION_TYPE_CFG])
            self.guiCompletionType.setItemSelected(item, True)

    def defaults(self):
        self.scrollbackLinesCount.setValue(10000)
        completionDefault = self.guiCompletionType.item(0)
        self.guiCompletionType.setItemSelected(completionDefault, True)


class ConfigPage(kate.Kate.PluginConfigPage, QWidget):
    """Kate configuration page for this plugin."""
    def __init__(self, parent=None, name=None):
        super(ConfigPage, self).__init__(parent, name)
        self.widget = ConfigWidget(parent)
        lo = parent.layout()
        lo.addWidget(self.widget)

    def apply(self):
        self.widget.apply()

    def reset(self):
        self.widget.reset()

    def defaults(self):
        self.widget.defaults()
        self.changed.emit()


@kate.configPage('IPython Console', 'IPython Console Settings', icon='text-x-python')
def configPage(parent=None, name=None):
    return ConfigPage(parent, name)


@kate.init
def init():
    kate_window = kate.mainInterfaceWindow()
    v = kate_window.createToolView('ipython_console', kate_window.Bottom, kate.gui.loadIcon('text-x-python'), 'IPython Console')
    console = terminal_widget(parent=v, kate=kate)
    # Load CSS from file '${appdir}/ipython_console.css'
    css_string = None
    search_dirs = kate.applicationDirectories() + [os.path.dirname(__file__)]
    for appdir in search_dirs:
        # TODO Make a CSS file configurable?
        css_file = os.path.join(appdir, _CONSOLE_CSS)
        if os.path.isfile(css_file):
            try:
                with open(css_file, 'r') as f:
                    css_string = f.read()
                    break
            except IOError:
                pass

    if css_string:
        console.style_sheet = css_string

    if _SCROLLBACK_LINES_COUNT_CFG in kate.configuration:
        console.buffer_size = kate.configuration[_SCROLLBACK_LINES_COUNT_CFG]

# kate: space-indent on; indent-width 4;
