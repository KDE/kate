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
from PyKDE4.ktexteditor import KTextEditor
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
    # Completion string minimum size.
    #
    keySize = None
    #
    # Original file prefix.
    #
    srcIn = None
    #
    # Replacement file prefix.
    #
    srcOut = None

    def __init__(self, parent = None, name = None):
	super(ConfigWidget, self).__init__(parent)

	lo = QGridLayout()
	self.setLayout(lo)

	self.idFile = KLineEdit()
	self.idFile.setWhatsThis(i18n("Name of ID file."))
	lo.addWidget(self.idFile, 0, 1, 1, 1)
	self.labelIdFile = QLabel(i18n("&ID file:"))
	self.labelIdFile.setBuddy(self.idFile)
	lo.addWidget(self.labelIdFile, 0, 0, 1, 1)

	self.keySize = QSpinBox()
	self.keySize.setMinimum(3)
	self.keySize.setMaximum(16)
	self.keySize.setWhatsThis(i18n("Minimum length of token before completions will be shown."))
	lo.addWidget(self.keySize, 0, 3, 1, 1)
	self.labelKeySize = QLabel(i18n("&Complete after:"))
	self.labelKeySize.setBuddy(self.keySize)
	lo.addWidget(self.labelKeySize, 0, 2, 1, 1)

	self.srcIn = KLineEdit()
	self.srcIn.setWhatsThis(i18n("If not empty, when looking in a file for matches, replace this prefix of the file name."))
	lo.addWidget(self.srcIn, 1, 1, 1, 3)
	self.labelSrcIn = QLabel(i18n("&Replace:"))
	self.labelSrcIn.setBuddy(self.srcIn)
	lo.addWidget(self.labelSrcIn, 1, 0, 1, 1)

	self.srcOut = KLineEdit()
	self.srcOut.setWhatsThis(i18n("Replacement prefix. Use %i to insert the prefix of the ID file."))
	lo.addWidget(self.srcOut, 2, 1, 1, 3)
	self.labelSrcOut = QLabel(i18n("&With this:"))
	self.labelSrcOut.setBuddy(self.srcOut)
	lo.addWidget(self.labelSrcOut, 2, 0, 1, 1)

	self.reset();

    def apply(self):
	kate.configuration["idFile"] = self.idFile.text().encode("utf-8")
	kate.configuration["keySize"] = self.keySize.value()
	kate.configuration["srcIn"] = self.srcIn.text().encode("utf-8")
	kate.configuration["srcOut"] = self.srcOut.text().encode("utf-8")
	kate.configuration.save()

    def reset(self):
	self.defaults()
	if "idFile" in kate.configuration:
	    self.idFile.setText(kate.configuration["idFile"])
	if "keySize" in kate.configuration:
	    self.keySize.setValue(kate.configuration["keySize"])
	if "srcIn" in kate.configuration:
	    self.srcIn.setText(kate.configuration["srcIn"])
	if "srcOut" in kate.configuration:
	    self.srcOut.setText(kate.configuration["srcOut"])

    def defaults(self):
	self.idFile.setText("/view/myview/vob/ID")
	self.keySize.setValue(5)
	self.srcIn.setText("/vob")
	self.srcOut.setText("%i/vob")

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

class ConfigDialog(KDialog):
    """Standalong configuration dialog for this plugin."""
    def __init__(self, parent = None):
	super(ConfigDialog, self).__init__(parent)
	self.widget = ConfigWidget(self)
	self.setMainWidget(self.widget)
	self.setButtons(KDialog.ButtonCode(KDialog.Default | KDialog.Reset | KDialog.Ok | KDialog.Cancel))
	self.applyClicked.connect(self.apply)
	self.resetClicked.connect(self.reset)
	self.defaultClicked.connect(self.defaults)

    @pyqtSlot()
    def apply(self):
	self.widget.apply()

    @pyqtSlot()
    def reset(self):
	self.widget.reset()

    @pyqtSlot()
    def defaults(self):
	self.widget.defaults()

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
		for text in open(fileName):
		    column = text.find(token)
		    if column > -1:
			resultRow = list()
			resultRow.append(QStandardItem(text[:-1]))
			resultRow.append(QStandardItem(str(line + 1)))
			resultRow.append(QStandardItem(str(column + 1)))
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
	self.tree.setEditTriggers(QAbstractItemView.NoEditTriggers)
	self.tree.setSelectionBehavior(QAbstractItemView.SelectRows)
	self.tree.setSelectionMode(QAbstractItemView.SingleSelection)
	self.tree.setItemsExpandable(False)

	self.token.setCompletionMode(KGlobalSettings.CompletionPopupAuto)
	self.token.returnPressed.connect(self.literalSearch)
	self.token.completion.connect(self._findCompletion)
	self.asyncFill.connect(self._continueCompletion)
	self.token.completionObject().clear();
	self.tree.doubleClicked.connect(self._treeClicked)

    def __del__(self):
	"""Plugins that use a toolview need to delete it for reloading to work."""
	if self.toolView:
	    self.toolView.deleteLater()
	    self.toolView = None

    @pyqtSlot("QString")
    def literalSearch(self, token):
	try:
	    self.model.literalSearch(token)
	    self.tree.expandAll()
	except IOError as detail:
	    KMessageBox.error(self.parent(), str(detail), i18n("Error finding {}").format(token))

    def _findCompletion(self, token):
	"""Fill the completion object with potential matches."""
	completionObj = self.token.completionObject()
	completionObj.clear()
	if len(token) < kate.configuration["keySize"]:
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

    @pyqtSlot("QModelIndex &")
    def _treeClicked(self, index):
	"""Fill the completion object with potential matches.

	TODO: make this asynchronous, and able to be interrupted.
	"""
	parent = index.parent()
	if parent.isValid():
	    #
	    # We got a specific search result.
	    #
	    file = parent.data()
	    line = int(index.sibling(index.row(), 1).data()) - 1
	    column = int(index.sibling(index.row(), 2).data()) - 1
	else:
	    #
	    # We got a file.
	    #
	    file, line, column = index.data(), 0, 0
	#
	# Navigate to the point in the file.
	#
	document = kate.documentManager.openUrl(KUrl.fromPath(file))
	kate.mainInterfaceWindow().activateView(document)
	point = KTextEditor.Cursor(line, column)
	kate.activeView().setCursorPosition(point)

    def show(self):
	#
	# Establish our configuration. Loop until we have a viable idFile, or
	# the user cancels.
	#
	fileSet = False
	while not fileSet:
	    dialog = ConfigDialog(kate.mainWindow().centralWidget())
	    status = dialog.exec_()
	    if status == QDialog.Rejected:
		break
	    dialog.widget.apply()
	    try:
		searchBar.dataSource.setFile(kate.configuration["idFile"])
		fileSet = True
	    except IOError as detail:
		KMessageBox.error(self.parent(), str(detail), i18n("ID database error"))
	if fileSet:
	    kate.mainInterfaceWindow().showToolView(self.toolView)

    def hide(self):
	kate.mainInterfaceWindow().hideToolView(self.toolView)

@kate.action("gid", shortcut = "Ctrl+/", menu = "&Gid", icon = "edit-find")
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
