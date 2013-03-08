"""IPython console for hacking kate and doing science

Uses this: http://stackoverflow.com/a/11525205/247482

May need reworking with IPython 1.0 due to changed event loop
"""

# TODO: fix help() by fixing input()

# TODO Get an icon for config page and toolview (from ipython package)

import kate

import os
import sys
sys.argv = [__file__]

import atexit

from IPython.zmq.ipkernel import IPKernelApp
from IPython.lib.kernel import find_connection_file
from IPython.frontend.qt.kernelmanager import QtKernelManager
from IPython.frontend.qt.console.rich_ipython_widget import RichIPythonWidget

from PyQt4 import uic
from PyQt4.QtGui import *

_SCROLLBACK_LINES_COUNT_CFG = 'ipythonConsole:scrollbackLinesCount'
_CONFIG_UI = 'python_console_ipython.ui'
_CONSOLE_CSS = 'python_console_ipython.css'


def event_loop(kernel):
    kernel.timer = kate.gui.QTimer()
    kernel.timer.timeout.connect(kernel.do_one_iteration)
    kernel.timer.start(1000 * kernel._poll_interval)

def default_kernel_app():
    app = IPKernelApp.instance()
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

def terminal_widget(parent=None, **kwargs):
    kernel_app = default_kernel_app()
    manager = default_manager(kernel_app)
    widget = RichIPythonWidget(parent=parent, gui_completion='droplist')
    widget.kernel_manager = manager

    #update namespace
    kernel_app.shell.user_ns.update(kwargs)
    kernel_app.shell.user_ns['console'] = widget
    kernel_app.shell.run_cell(
        'print("\\nAvailable variables are everything from pylab, “{}”, and this console as “console”")'
        .format('”, “'.join(kwargs.keys())))

    kernel_app.start()
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

        self.reset();

    def apply(self):
        kate.configuration[_SCROLLBACK_LINES_COUNT_CFG] = self.scrollbackLinesCount.value()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if _SCROLLBACK_LINES_COUNT_CFG in kate.configuration:
            self.scrollbackLinesCount.setValue(kate.configuration[_SCROLLBACK_LINES_COUNT_CFG])

    def defaults(self):
        self.scrollbackLinesCount.setValue(10000)


class ConfigPage(kate.Kate.PluginConfigPage, QWidget):
    """Kate configuration page for this plugin."""
    def __init__(self, parent = None, name = None):
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


@kate.configPage("IPython Console", "IPython Console Settings", icon="applications-development")
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
