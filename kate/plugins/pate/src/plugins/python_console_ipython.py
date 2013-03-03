"""IPython console for hacking kate and doing science

Uses this: http://stackoverflow.com/a/11525205/247482

May need reworking with IPython 1.0 due to changed event loop
"""

#TODO: fix help() by fixing input()

import kate

import sys
sys.argv = [__file__]

import atexit

from IPython.zmq.ipkernel import IPKernelApp
from IPython.lib.kernel import find_connection_file
from IPython.frontend.qt.kernelmanager import QtKernelManager
from IPython.frontend.qt.console.rich_ipython_widget import RichIPythonWidget

def event_loop(kernel):
	kernel.timer = kate.gui.QTimer()
	kernel.timer.timeout.connect(kernel.do_one_iteration)
	kernel.timer.start(1000 * kernel._poll_interval)

def default_kernel_app():
	app = IPKernelApp.instance()
	app.initialize(['python'])
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

@kate.init
def init():
	kate_window = kate.mainInterfaceWindow()
	v = kate_window.createToolView('ipython_console', kate_window.Bottom, kate.gui.loadIcon('text-x-python'), 'IPython Console')
	console = terminal_widget(parent=v, kate=kate)
	console.setStyleSheet('RichIPythonWidget {background-color: transparent}')
