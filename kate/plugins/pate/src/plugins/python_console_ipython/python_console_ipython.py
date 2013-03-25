# -*- coding: utf-8 -*-
"""IPython console for hacking kate and doing science

Uses this: http://stackoverflow.com/a/11525205/247482

May need reworking with IPython 1.0 due to changed event loop
"""

# TODO: fix help() by fixing input()

import kate

import os
import sys
sys.argv = [__file__]


NEED_PACKAGES = {}

try:
    import IPython
except ImportError:
    NEED_PACKAGES["ipython"] = "0.13.1"

try:
    from IPython import zmq
except ImportError:
    if sys.version_info.major == 3:
        NEED_PACKAGES["pyzmq"] = "13.0.0"
    else:
        NEED_PACKAGES["pyzmq"] = "2.0.10.1"

try:
    import readline
except ImportError:
    NEED_PACKAGES["readline"] = "6.2.4.1"

try:
    import pygments
except ImportError:
    NEED_PACKAGES["Pygments"] = "1.6"


if NEED_PACKAGES:
    msg = "You need install the next packages:\n"
    for package in NEED_PACKAGES:
        msg += "\t\t%(package)s. Use easy_install %(package)s==%(version)s \n" % {'package': package,
                                                                                  'version':   NEED_PACKAGES[package]}
    raise ImportError(msg)


import atexit

from copy import copy

from IPython.zmq.ipkernel import IPKernelApp
from IPython.lib.kernel import find_connection_file
from IPython.frontend.qt.kernelmanager import QtKernelManager
from IPython.frontend.qt.console.rich_ipython_widget import RichIPythonWidget

from PyQt4 import uic
from PyQt4.QtGui import QWidget

_SCROLLBACK_LINES_COUNT_CFG = 'ipythonConsole:scrollbackLinesCount'
_GUI_COMPLETION_TYPE_CFG = 'ipythonConsole:guiCompletionType'
_CONFIG_UI = 'python_console_ipython.ui'
_CONSOLE_CSS = 'python_console_ipython.css'
_GUI_COMPLETION_CONVERT = {0: "droplist",
                           1: None}


def event_loop(kernel):
    kernel.timer = kate.gui.QTimer()
    kernel.timer.timeout.connect(kernel.do_one_iteration)
    kernel.timer.start(1000 * kernel._poll_interval)


def default_kernel_app():
    app = IPKernelApp.instance()
    if not app.config:
        app.initialize(['python', '--pylab=qt'])
    app.kernel.eventloop = event_loop
    return app


def default_manager(kernel_app):
    connection_file = find_connection_file(kernel_app.connection_file)
    manager = QtKernelManager(connection_file=connection_file)
    manager.load_connection_file()
    manager.start_channels()
    atexit.register(manager.cleanup_connection_file)
    return manager


def getProjectPlugin():
    mainWindow = kate.mainInterfaceWindow()
    projectPluginView = mainWindow.pluginView("kateprojectplugin")
    return projectPluginView


def django_project_filename_changed(kernel_app):
    # Uses this: https://github.com/django-extensions/django-extensions/blob/master/django_extensions/management/shells.py#L7
    from django.db.models.loading import get_models, get_apps
    loaded_models = get_models()  # NOQA

    from django.conf import settings
    imported_objects = {'settings': settings}

    dont_load = getattr(settings, 'SHELL_PLUS_DONT_LOAD', [])
    model_aliases = getattr(settings, 'SHELL_PLUS_MODEL_ALIASES', {})
    print_imports = ""
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
            try:
                imported_object = getattr(__import__(app_mod.__name__, {}, {}, model.__name__), model.__name__)
                model_name = model.__name__
                if "%s.%s" % (app_name, model_name) in dont_load:
                    continue
                alias = app_aliases.get(model_name, model_name)
                imported_objects[alias] = imported_object
                if model_name == alias:
                    model_labels.append(model_name)
                else:
                    model_labels.append("%s (as %s)" % (model_name, alias))
            except AttributeError as e:
                kernel_app.shell.run_cell('print("Failed to import \'%s\' from \'%s\' reason: %s)' % (model.__name__, app_name, str(e)))
                continue
        print_imports += 'print("From \'%s\' autoload: %s");' % (app_mod.__name__.split('.')[-2], ", ".join(model_labels)) 
    kernel_app.shell.run_cell(print_imports)
    return imported_objects


def can_load_project(version):
    if not version:
        return True
    version_info = version.split(".")
    if len(version_info) == 0:
        return False
    elif len(version_info) == 1:
        return int(version_info[0]) == sys.version_info.major
    elif len(version_info) == 2:
        return int(version_info[0]) == sys.version_info.major and \
            int(version_info[1]) == sys.version_info.minor
    else:
        return int(version_info[0]) == sys.version_info.major and \
            int(version_info[1]) == sys.version_info.minor and \
            int(version_info[2]) == sys.version_info.micro


def projectFileNameChanged(*args, **kwargs):
    projectPlugin = getProjectPlugin()
    projectMap = projectPlugin.property("projectMap")
    if "python" in projectMap:
        projectName = projectPlugin.property("projectName")
        version = projectMap.get("version", None)
        kernel_app = default_kernel_app()
        # Check Python version
        if not can_load_project(version):
            msg = 'print("Can not load this project: %s. Python Version incompatible")' % projectName
            kernel_app.shell.run_cell(msg)
            sys.stdout.flush()
        kernel_app.shell.reset()
        kernel_app.shell.run_cell('print("Change project %s")' % projectName)
        projectMapPython = projectMap["python"]
        extraPath = projectMapPython.get("extraPath", [])
        environs = projectMapPython.get("environs", {})
        # Add Extra path
        if not getattr(sys, "original_path", None):
            sys.original_path = copy(sys.path)
            sys.original_modules = sys.modules.keys()
        else:
            for module in sys.modules.keys():
                if not module in sys.original_modules:
                    del sys.modules[module]
        sys.path = extraPath + sys.original_path
        sys.path_importer_cache = {}
        # Add environs
        for key, value in environs.items():
            if key in os.environ:
                os.environ.pop(key)
            os.environ.setdefault(key, value)
        # Special treatment
        if projectMapPython.get("projectType", "").lower() == "django":
            kernel_app = default_kernel_app()
            imported_objects = django_project_filename_changed(kernel_app)
            kernel_app.shell.user_ns.update(imported_objects)
        # Print details
        kernel_app.start()
        sys.stdout.flush()


def terminal_widget(parent=None, **kwargs):
    kernel_app = default_kernel_app()
    manager = default_manager(kernel_app)
    try:
        gui_completion = _GUI_COMPLETION_CONVERT[kate.configuration[_GUI_COMPLETION_TYPE_CFG]]
    except KeyError:
        gui_completion = 'droplist'
    if gui_completion:
        widget = RichIPythonWidget(parent=parent, gui_completion=gui_completion)
    else:
        widget = RichIPythonWidget(parent=parent)
    widget.kernel_manager = manager

    #update namespace
    kernel_app.shell.user_ns.update(kwargs)
    kernel_app.shell.user_ns['console'] = widget
    kernel_app.shell.run_cell(
        'print("\\nAvailable variables are everything from pylab, “{}”, and this console as “console”")'
        .format('”, “'.join(kwargs.keys())))
    projectPlugin = getProjectPlugin()
    if projectPlugin:
        projectPlugin.projectFileNameChanged.connect(projectFileNameChanged)
        projectFileNameChanged()
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


@kate.configPage("IPython Console", "IPython Console Settings", icon="text-x-python")
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
