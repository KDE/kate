# Expose (a subset of) the reading functionality GNU idutils within Kate.
#
# Copyright (C) 2012 Shaheed Haque <srhaque@theiet.org>
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

__title__ = "gid"
__author__ = "Shaheed Haque <srhaque@theiet.org>"
__license__ = "LGPL"

import kate
import kate.gui

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyKDE4.kdecore import *
from PyKDE4.kdeui import *
from idutils import Lookup

import sip

searchBar = None

class ConfigWidget(QWidget):
    """Configuration widget for this plugin."""
    #
    # Location of ID file.
    #
    idFile = None
    #
    # Original file prefix.
    #
    srcIn = None
    #
    # Replacement file prefix.
    # ...with this.
    #
    srcOut = None

    def __init__(self, parent = None, name = None):
	super(ConfigWidget, self).__init__(parent)

	lo = QGridLayout()
	self.setLayout(lo)

	self.idFile = KLineEdit()
	self.idFile.setWhatsThis(i18n("Location of ID file."))
	lo.addWidget(self.idFile, 0, 1, 1, 3)
	self.labelIdFile = QLabel(i18n("&ID file:"))
	self.labelIdFile.setBuddy(self.idFile)
	lo.addWidget(self.labelIdFile, 0, 0, 1, 1)

	self.srcIn = KLineEdit()
	self.srcIn.setWhatsThis(i18n("If not empty, when looking in a file for matches, replace this prefix of the file name."))
	lo.addWidget(self.srcIn, 2, 1, 1, 3)
	self.labelSrcIn = QLabel(i18n("&Replace:"))
	self.labelSrcIn.setBuddy(self.srcIn)
	lo.addWidget(self.labelSrcIn, 2, 0, 1, 1)

	self.srcOut = KLineEdit()
	self.srcOut.setWhatsThis(i18n("Replacement prefix."))
	lo.addWidget(self.srcOut, 3, 1, 1, 3)
	self.labelSrcOut = QLabel(i18n("&With this:"))
	self.labelSrcOut.setBuddy(self.srcOut)
	lo.addWidget(self.labelSrcOut, 3, 0, 1, 1)

	self.reset();

    def apply(self):
	kate.configuration["idFile"] = self.idFile.text()
	kate.configuration["srcIn"] = self.srcIn.text()
	kate.configuration["srcOut"] = self.srcOut.text()
	kate.configuration.save()

    def reset(self):
	self.defaults()
	if "idFile" in kate.configuration:
	    self.idFile.setText(kate.configuration["idFile"])
	if "srcIn" in kate.configuration:
	    self.srcIn.setText(kate.configuration["srcIn"])
	if "srcOut" in kate.configuration:
	    self.srcIn.setText(kate.configuration["srcOut"])

    def defaults(self):
	self.idFile.setText("/vob/ios/sys/ID")
	self.srcIn.setText("/vob")
	self.srcOut.setText("/vob")
	#self.changed.emit()

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
	self.widget.default()
	self.changed.emit()

class ConfigDialog(KDialog):
    """Standalong configuration dialog for this plugin."""
    def __init__(self, parent = None):
	super(ConfigDialog, self).__init__(parent)
	self.widget = ConfigWidget(self)
	self.setMainWidget(self.widget)

class TreeModel(QStandardItemModel):
    def __init__(self, dataSource):
	super(TreeModel, self).__init__()
	self.dataSource = dataSource
	self.setHorizontalHeaderLabels((i18n("Match"), i18n("Line"), i18n("Col")))

    def literalSearch(self, token):
	root = self.invisibleRootItem()
	root.removeRows(0, root.rowCount())
	try:
	    tokenFlags, hitCount, files = self.dataSource.literalSearch(token)
	    #
	    # For each file, list the lines where a match is found.
	    #
	    for fileName, fileFlags in files:
		fileRow = QStandardItem(fileName)
		line = 0
		for text in open("idfile.py.safe2"):
		    column = text.find("dpss")
		    if column > -1:
			resultRow = list()
			resultRow.append(QStandardItem(text[:-1]))
			resultRow.append(QStandardItem(str(line)))
			resultRow.append(QStandardItem(str(column)))
			fileRow.appendRow(resultRow)
		    line += 1
		root.appendRow(fileRow)
	except IndexError:
	    return

class SearchBar(QObject):
    toolView = None
    dataSource = None
    lastToken = None
    lastOffset = None
    lastName = None

    #
    asyncFill = pyqtSignal("QString")

    def __init__(self, parent):
	super(SearchBar, self).__init__(parent)
	self.dataSource = Lookup()
	self.toolView = kate.mainInterfaceWindow().createToolView("idutils_gid_plugin", kate.Kate.MainWindow.Bottom, SmallIcon("edit-find"), i18n("gid Search"))
	# By default, the toolview has box layout, which is not easy to delete.
	# For now, just add an extra widget.
	top = QWidget(self.toolView)
	lo = QGridLayout(top)
	top.setLayout(lo)

	self.token = KLineEdit()
	lo.addWidget(self.token, 0, 1, 1, 1)
	self.labelToken = QLabel(i18n("&Token:"))
	self.labelToken.setBuddy(self.token)
	lo.addWidget(self.labelToken, 0, 0, 1, 1)

	self.keySize = QSpinBox()
	self.keySize.setValue(5)
	self.keySize.setMinimum(3)
	self.keySize.setMaximum(16)
	self.keySize.setWhatsThis(i18n("The length of a token before completions will be shown."))
	lo.addWidget(self.keySize, 0, 3, 1, 1)
	self.labelKeySize = QLabel(i18n("&Lookup after:"))
	self.labelKeySize.setBuddy(self.keySize)
	lo.addWidget(self.labelKeySize, 0, 2, 1, 1)

	self.tree = QTreeView()
	lo.addWidget(self.tree, 1, 0, 1, 4)
	self.model = TreeModel(self.dataSource)
	self.tree.setModel(self.model)
	if parent.width() > 680:
	    width = parent.width() - 180
	else:
	    width = 500
	self.tree.setColumnWidth(0, width)
	self.tree.setColumnWidth(1, 50)
	self.tree.setColumnWidth(2, 50)

	self.token.setCompletionMode(KGlobalSettings.CompletionPopupAuto)
	self.token.returnPressed.connect(self.model.literalSearch)
	self.token.returnPressed.connect(self.tree.expandAll)
	self.token.completion.connect(self._findCompletion)
	self.asyncFill.connect(self._continueCompletion)
	self.token.completionObject().clear();

    def __del__(self):
	"""Plugins that use a toolview need to delete it for reloading to work."""
	if self.toolView:
	    self.toolView.deleteLater()
	    self.toolView = None

    def _findCompletion(self, token):
	"""Fill the completion object with potential matches."""
	completionObj = self.token.completionObject()
	completionObj.clear();
	if len(token) < self.keySize.value():
	    #
	    # Don't try to match if the token is too short.
	    #
	    return
	#
	# Add the first batch of items synchronously.
	#
	name = self._firstMatch(token)
	if name:
	    completionObj.addItem(name)
	    self._continueCompletion(token)

    @pyqtSlot("QString")
    def _continueCompletion(self, token):
	"""Fill the completion object with potential matches.

	TODO: make this asynchronous, and able to be interrupted.
	"""
	matches = 0
	completionObj = self.token.completionObject()
	name = self._nextMatch()
	while name and matches < 50:
	    matches += 1
	    completionObj.addItem(name)
	    name = self._nextMatch()
	if name:
	    #
	    # Go round for more.
	    #
	    self.asyncFill.emit(token)

    def _firstMatch(self, token):
	try:
	    self.lastToken = token
	    self.lastOffset, self.lastName = self.dataSource.prefixSearchFirst(self.lastToken)
	    return self.lastName
	except IndexError:
	    return None

    def _nextMatch(self):
	try:
	    self.lastOffset, self.lastName = self.dataSource.prefixSearchNext(self.lastOffset, self.lastToken)
	    return self.lastName
	except IndexError:
	    return None

    def show(self):
	#
	# Loop until we have a viable idFile, or the user cancels.
	#
	fileSet = False
	while "idFile" not in kate.configuration or not fileSet:
	    dialog = ConfigDialog(kate.mainWindow().centralWidget())
	    status = dialog.exec_()
	    if status == QDialog.Rejected:
		break
	    dialog.widget.apply()
	    try:
		searchBar.dataSource.setFile(kate.configuration["idFile"])
		fileSet = True
	    except Exception as (errno, strerror):
		KMessageBox.information(self.parent(),
				      "Error {} ({}): {}".format(kate.configuration["idFile"], errno, strerror))
	if fileSet:
	    kate.mainInterfaceWindow().showToolView(self.toolView)

    def hide(self):
	kate.mainInterfaceWindow().hideToolView(self.toolView)

@kate.action("gid", shortcut = "Ctrl+/", menu = "edit", icon = "edit-find")
def show():
    global searchBar
    if searchBar is None:
	searchBar = SearchBar(kate.mainWindow())
    searchBar.show()

@kate.configPage("gid", "gid Lookup", icon = "edit-find")
def configPage(parent = None, name = None):
    return ConfigPage(parent, name)

@kate.unload
def destroy():
    """Plugins that use a toolview need to delete it for reloading to work."""
    global searchBar
    if searchBar:
	searchBar.__del__()
	searchBar = None
