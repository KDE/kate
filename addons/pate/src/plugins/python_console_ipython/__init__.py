# -*- coding: utf-8 -*-
"""IPython console for hacking kate and doing science"""

from .python_console_ipython import *


consoleToolView = None

@kate.init
def init():
    # Make an instance of a console tool view
    global consoleToolView
    if consoleToolView is None:
        consoleToolView = ConsoleToolView(kate.mainWindow())


@kate.unload
def destroy():
    '''Plugins that use a toolview need to delete it for reloading to work.'''
    global consoleToolView
    if consoleToolView:
        del consoleToolView
        consoleToolView = None
