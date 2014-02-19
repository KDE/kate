# -*- coding: utf-8 -*-
"""IPython console for hacking kate and doing science"""

# Uses this for IPython 0.13: http://stackoverflow.com/a/11525205/247482
# TODO: remove Ipython 0.13 support as soon as IPython 1.0 final is released:
#       IPython 1 is much nicer in code, stability, and it handles input() and help()

from __future__ import unicode_literals

import sys

from PyQt4.QtCore import QEvent, QObject, Qt, QTimer
from PyKDE4.kdecore import i18nc, i18n as _translate
from PyKDE4.kdeui import KIcon
from libkatepate.compat import text_type

sys.argv = [__file__]


# TODO Remove this crap!
def i18n(msg, *args):
    if isinstance(msg, text_type):
        msg = msg.encode('utf-8')
    return _translate(msg, *args) if args else _translate(msg)


import kate
import os

try:  # ≥1.0
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


_SCROLLBACK_LINES_COUNT_CFG = 'ipythonConsole:scrollbackLinesCount'
_GUI_COMPLETION_TYPE_CFG = 'ipythonConsole:guiCompletionType'
_CONFIG_UI = 'python_console_ipython.ui'
_CONSOLE_CSS = 'python_console_ipython.css'
_GUI_COMPLETION_CONVERT = ['droplist', None]


def init_ipython():
    """
    Encapsulate Kernel creation logic: Only the kernel client, manager and shell are exposed
    This is in order to ensure interoperability between major IPython changes
    """
    if ipython_1:
        manager = QtInProcessKernelManager()
        manager.start_kernel()
        manager.kernel.gui = 'qt4'
        shell = manager.kernel.shell
        shell.run_cell('%pylab inline')
        client = manager.client()
        client.start_channels()
    else:
        def event_loop(kernel):
            kernel.timer = QTimer()
            kernel.timer.timeout.connect(kernel.do_one_iteration)
            kernel.timer.start(1000 * kernel._poll_interval)

        kernel_app = IPKernelApp.instance()
        kernel_app.initialize(['python', '--pylab=qt'])     # at this point, print() won’t work anymore
        kernel_app.kernel.eventloop = event_loop

        connection_file = find_connection_file(kernel_app.connection_file)
        manager = QtKernelManager(connection_file=connection_file)
        manager.load_connection_file()
        manager.start_channels()

        kernel_app.start()

        client = None
        shell = kernel_app.shell

    return client, manager, shell


def insert_django_objects(shell):
    """ Imports settings and models into the shell """
    # Uses this: https://github.com/django-extensions/django-extensions/blob/master/django_extensions/management/shells.py#L7
    from django.db.models.loading import get_models, get_apps
    loaded_models = get_models()                            # NOQA

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
                msg = i18n('\nFailed to import “%1” from “%2” reason: %3', model_name, app_name, reason)
                shell.write(msg)
                continue
        msg = i18n('\nFrom “%1” autoload: %2', app_mod.__name__.split('.')[-2], ', '.join(model_labels))
        imports.append(msg)
    for import_msg in imports:
        shell.write(import_msg)
    shell.user_ns.update(imported_objects)


def reset_shell(shell, base_ns):
    """ Resets shell and loads project settings for Python """
    projectPlugin = get_project_plugin()
    projectMap = projectPlugin.property('projectMap')
    if 'python' in projectMap:
        projectName = projectPlugin.property('projectName')
        projectMapPython = projectMap['python']
        # Check Python version
        version = projectMapPython.get('version', None)
        if not is_version_compatible(version):
            msg = i18n('\n\nCannot load this project: %1. Python Version incompatible', projectName)
            shell.write(msg)
            sys.stdout.flush()
            return

        shell.reset()
        shell.user_ns.update(base_ns)

        shell.write(i18n('\n\nLoad project: %1', projectName))
        extraPath = projectMapPython.get('extraPath', [])
        environs = projectMapPython.get('environs', {})
        # Add Extra path
        add_extra_path(extraPath)
        # Add environs
        add_environs(environs)
        # Special treatment
        if projectMapPython.get('projectType', '').lower() == 'django':
            insert_django_objects(shell)
        # Print details
        sys.stdout.flush()


def make_terminal_widget(parent=None, **base_ns):
    try:
        gui_completion = _GUI_COMPLETION_CONVERT[kate.configuration[_GUI_COMPLETION_TYPE_CFG]]
    except KeyError:
        gui_completion = 'droplist'
    if gui_completion:
        widget = RichIPythonWidget(parent=parent, gui_completion=gui_completion)
    else:
        widget = RichIPythonWidget(parent=parent)

    widget.banner += i18n('\nAvailable variables are everything from pylab, “%1”, and this console as “console”', '”, “'.join(base_ns.keys()))

    client, manager, shell = init_ipython()

    # https://github.com/ipython/ipython/blob/master/examples/inprocess/embedded_qtconsole.py
    if ipython_1:
        widget.kernel_client = client

        widget.exit_requested.connect(widget.kernel_client.stop_channels)
        widget.exit_requested.connect(manager.shutdown_kernel)
    else:
        widget.exit_requested.connect(manager.cleanup_connection_file)

    widget.kernel_manager = manager

    base_ns['console'] = widget
    shell.user_ns.update(base_ns)

    try:
        projectPlugin = get_project_plugin()
    except AttributeError:
        projectPlugin = None
    if projectPlugin:
        def project_changed(*args, **kwargs):
            reset_shell(shell, base_ns)
        projectPlugin.projectFileNameChanged.connect(project_changed)
        project_changed()

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
        kate.configuration[_GUI_COMPLETION_TYPE_CFG] = self.guiCompletionType.currentIndex()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if _SCROLLBACK_LINES_COUNT_CFG in kate.configuration:
            self.scrollbackLinesCount.setValue(kate.configuration[_SCROLLBACK_LINES_COUNT_CFG])
        if _GUI_COMPLETION_TYPE_CFG in kate.configuration:
            has_valid_type = isinstance(kate.configuration[_GUI_COMPLETION_TYPE_CFG], int)
            has_valid_value = kate.configuration[_GUI_COMPLETION_TYPE_CFG] < 2
            if has_valid_type and has_valid_value:
                self.guiCompletionType.setCurrentIndex(kate.configuration[_GUI_COMPLETION_TYPE_CFG])

    def defaults(self):
        self.scrollbackLinesCount.setValue(10000)
        self.guiCompletionType.setCurrentIndex(0)


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


@kate.configPage(
    i18nc('@item:inmenu', 'IPython Console')
  , i18nc('@title:group', 'IPython Console Settings')
  , icon='text-x-python'
  )
def configPage(parent=None, name=None):
    return ConfigPage(parent, name)


class ConsoleToolView(QObject):

    def __init__(self, parent):
        super(ConsoleToolView, self).__init__(parent)
        self.toolView = kate.mainInterfaceWindow().createToolView(
            'ipython_console',
            kate.Kate.MainWindow.Bottom,
            KIcon('text-x-python').pixmap(32,32),
            i18nc('@title:tab', 'IPython Console')
        )
        self.toolView.installEventFilter(self)
        self.console = make_terminal_widget(parent=self.toolView, kate=kate)

        # Load CSS from file '${appdir}/ipython_console.css'
        css_string = None
        search_dirs = kate.applicationDirectories() + [os.path.dirname(__file__)]
        for appdir in search_dirs:
            # TODO Make a CSS file configurable?
            css_file = os.path.join(appdir, _CONSOLE_CSS)
            try:
                with open(css_file, 'r') as f:
                    css_string = f.read()
                    break
            except IOError:
                pass

        if css_string:
            self.console.style_sheet = css_string
            if ipython_1:                                   # For seamless borders
                self.console.setStyleSheet(css_string)

        if _SCROLLBACK_LINES_COUNT_CFG in kate.configuration:
            self.console.buffer_size = kate.configuration[_SCROLLBACK_LINES_COUNT_CFG]

    def __del__(self):
        """Plugins that use a toolview need to delete it for reloading to work."""
        mw = kate.mainInterfaceWindow()
        if mw:
            mw.destroyToolView(self.toolView)
        self.console = None

    def eventFilter(self, obj, event):
        """Hide the IPython console tool view on ESCAPE key"""
        # TODO This doesn't work if cursor (focus) inside of ipython prompt :(
        if event.type() == QEvent.KeyPress and event.key() == Qt.Key_Escape:
            kate.mainInterfaceWindow().hideToolView(self.toolView)
            return True
        return self.toolView.eventFilter(obj, event)
