"""Browse the tokens in a GNU idutils ID file, and use it to navigate within a codebase.

The necessary parts of the ID file are held in memory to allow sufficient performance
for token completion, and When looking up the usage of a token, or jumping to the 
definition, etags(1) is used to locate the definition.
"""
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

import kate
import kate.gui

from PyQt4 import uic
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyKDE4.kdecore import *
from PyKDE4.kdeui import *
from PyKDE4.ktexteditor import KTextEditor

from idutils import Lookup

import codecs
import os.path
import re
import sip
import subprocess
import time

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

        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), "config.ui"), self)

        self.reset();

    def apply(self):
        kate.configuration["idFile"] = self.idFile.text()
        kate.configuration["keySize"] = self.keySize.value()
        kate.configuration["useEtags"] = self.useEtags.isChecked()
        kate.configuration["useSuffixes"] = self.useSuffixes.text()
        kate.configuration["srcIn"] = self.srcIn.text()
        kate.configuration["srcOut"] = self.srcOut.text()
        kate.configuration.save()

    def reset(self):
        self.defaults()
        if "idFile" in kate.configuration:
            self.idFile.setText(kate.configuration["idFile"])
        if "keySize" in kate.configuration:
            self.keySize.setValue(kate.configuration["keySize"])
        if "useEtags" in kate.configuration:
            self.useEtags.setChecked(kate.configuration["useEtags"])
        if "useSuffixes" in kate.configuration:
            self.useSuffixes.setText(kate.configuration["useSuffixes"])
        if "srcIn" in kate.configuration:
            self.srcIn.setText(kate.configuration["srcIn"])
        if "srcOut" in kate.configuration:
            self.srcOut.setText(kate.configuration["srcOut"])

    def defaults(self):
        self.idFile.setText("/view/myview/vob/ID")
        self.keySize.setValue(5)
        self.useEtags.setChecked(True)
        self.useSuffixes.setText(".h;.hxx")
        self.srcIn.setText("/vob")
        self.srcOut.setText("%{idPrefix}/vob")

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
        self.resize(600, 200)

    @pyqtSlot()
    def apply(self):
        self.widget.apply()

    @pyqtSlot()
    def reset(self):
        self.widget.reset()

    @pyqtSlot()
    def defaults(self):
        self.widget.defaults()

def transform(file):
    """Return the transformed file name."""
    transformationKey = kate.configuration["srcIn"]
    if len(transformationKey):
        #
        # A transformation of the file name is requested.
        #
        try:
            left, right = file.split(transformationKey, 1)
        except ValueError:
            #
            # No transformation is applicable.
            #
            return file
        percentI = kate.configuration["srcOut"].find("%{idPrefix}")
        if percentI > -1:
            insertLeft, discard = kate.configuration["idFile"].split(transformationKey, 1)
            discard, insertRight = kate.configuration["srcOut"].split("%{idPrefix}", 1)
            file = insertLeft + insertRight + right
        else:
            file = kate.configuration["srcOut"] + right
    return file

class TreeModel(QStandardItemModel):

    _boredomInterval = 20

    def __init__(self, dataSource):
        super(TreeModel, self).__init__()
        self.dataSource = dataSource
        self.setHorizontalHeaderLabels((i18n("Match"), i18n("Line"), i18n("Col")))

    def _etagSearch(self, token, fileName):
        """Use etags to find any definition in this file.

        Look for [ 0x7f, token, 0x1 ].
        """
        if not kate.configuration["useEtags"]:
            return None
        etagsCmd = ["etags", "-o", "-", fileName]
        etagBytes = subprocess.check_output(etagsCmd, stderr = subprocess.STDOUT)
        tokenBytes = bytearray(token.encode("utf-8"))
        tokenBytes.insert(0, 0x7f)
        tokenBytes.append(0x1)
        etagDefinition = etagBytes.find(tokenBytes)
        if etagDefinition > -1:
            #
            # The line number follows.
            #
            lineNumberStart = etagDefinition + len(tokenBytes)
            lineNumberEnd = etagBytes.find(",", lineNumberStart)
            return int(etagBytes[lineNumberStart:lineNumberEnd])
        if etagBytes.startswith(bytearray(etagsCmd[0])):
            #
            # An error message was printed starting with "etags".
            #
            raise IOError(unicode(etagBytes, "latin-1"))
        return None

    def _scanFile(self, root, regexp, filterRe, token, fileName, isDeclaration):
        """Scan a file looking for interesting hits. Return the QModelIndex of the last of any definitions we find."""
        definitionIndex = None
        fileRow = None
        hits = 0
        line = 1
        try:
            definitionLine = self._etagSearch(token, fileName)
            #
            # Question: what encoding is this file? TODO A better approach
            # to this question.
            #
            for text in codecs.open(fileName, encoding="latin-1"):
                match = regexp.search(text)
                if match and filterRe:
                    match = filterRe.search(text)
                if match:
                    if hits == 0:
                        fileRow = QStandardItem(fileName)
                        if isDeclaration:
                            fileRow.setIcon(KIcon("text-x-chdr"))
                        root.appendRow(fileRow)
                    hits += 1
                    resultRow = list()
                    resultRow.append(QStandardItem(text[:-1]))
                    resultRow.append(QStandardItem(str(line)))
                    #
                    # The column value displayed by Kate is based on a
                    # virtual position, where TABs count as 8.
                    #
                    column = match.start()
                    tabs = text[:column].count("\t")
                    virtualColumn = QStandardItem(str(column + tabs * 7 + 1))
                    resultRow.append(virtualColumn)
                    virtualColumn.setData(column, Qt.UserRole + 1)
                    fileRow.appendRow(resultRow)
                    if line == definitionLine:
                        #
                        # Mark the line and the file as being a definition.
                        #
                        resultRow[0].setIcon(KIcon("go-jump-definition"))
                        fileRow.setIcon(KIcon("go-jump-definition"))
                        definitionIndex = resultRow[0].index()
                line += 1
        except IOError as e:
            fileRow.setIcon(KIcon("face-sad"))
            fileRow.appendRow(QStandardItem(str(e)))
        return definitionIndex

    def literalTokenSearch(self, parent, token, filter):
        """Add the entries which match the token to the tree, and return the QModelIndex of the last of any definitions we find.

        Entries are grouped under the file in which the hits are searched. Each
        entry shows the matched text, the line and column of the match. If so
        enabled, entries which are defintions according to etags are highlighted.

        If the output takes a long time to generate, the user is given options
        to continue or abort.
        """
        root = self.invisibleRootItem()
        root.removeRows(0, root.rowCount())
        self.definitionIndex = None
        try:
            tokenFlags, hitCount, files = self.dataSource.literalSearch(token)
        except IndexError:
            return None
        #
        # Compile the REs we need.
        #
        if len(filter):
            filter = filter.replace("%{token}", token)
            try:
                filterRe = re.compile(filter)
            except re.error:
                KMessageBox.error(parent.parent(), i18n("Filter '{}' is not a valid regular expression").format(filter), i18n("Invalid filter"))
                return None
        else:
            filterRe = None
        regexp = re.compile("\\b" + token + "\\b")
        if len(kate.configuration["useSuffixes"]) == 0:
            declarationRe = None
        else:
            declarationRe = kate.configuration["useSuffixes"].replace(";", "|")
            declarationRe = "(" + declarationRe.replace(".", "\.") + ")$"
            declarationRe = re.compile(declarationRe, re.IGNORECASE)
        startBoredomQuery = time.time()
        previousBoredomQuery = startBoredomQuery - self._boredomInterval / 2
        #
        # For each file, list the lines where a match is found.
        #
        definitionIndex = None
        filesListed = 0
        for fileName, fileFlags in files:
            fileName = transform(fileName)
            isDeclaration = declarationRe and declarationRe.search(fileName)
            #
            # Update the definitionIndex when we get a good one.
            #
            newDefinitionIndex = self._scanFile(root, regexp, filterRe, token, fileName, isDeclaration)
            if newDefinitionIndex:
                definitionIndex = newDefinitionIndex
            filesListed += 1
            #
            # Time to query the user's boredom level?
            #
            if time.time() - previousBoredomQuery > self._boredomInterval:
                r = KMessageBox.questionYesNoCancel(parent.parent(), i18n("Scanned {} of {} files in {} seconds").format(filesListed, len(files), int(time.time() - startBoredomQuery)),
                        i18n("Scan more files?"), KGuiItem(i18n("All Files")), KGuiItem(i18n("More Files")), KStandardGuiItem.cancel())
                if r == KMessageBox.Yes:
                    previousBoredomQuery = time.time() + 10 * self._boredomInterval
                elif r == KMessageBox.No:
                    previousBoredomQuery = time.time()
                else:
                    break
        #
        # Return the model index of the match.
        #
        return definitionIndex

class SearchBar(QObject):
    toolView = None
    dataSource = None
    lastToken = None
    lastOffset = None
    lastName = None
    gotSettings = False

    def __init__(self, parent):
        super(SearchBar, self).__init__(parent)
        self.dataSource = Lookup()
        self.toolView = kate.mainInterfaceWindow().createToolView("idutils_gid_plugin", kate.Kate.MainWindow.Bottom, SmallIcon("edit-find"), i18n("gid Search"))
        # By default, the toolview has box layout, which is not easy to delete.
        # For now, just add an extra widget.
        top = QWidget(self.toolView)

        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), "tool.ui"), top)
        self.token = top.token
        self.filter = top.filter
        self.settings = top.settings
        self.tree = top.tree
        self.model = TreeModel(self.dataSource)
        self.tree.setModel(self.model)
        width = parent.width() - 300
        self.tree.setColumnWidth(0, width - 100)
        self.tree.setColumnWidth(1, 50)
        self.tree.setColumnWidth(2, 50)

        self.token.setCompletionMode(KGlobalSettings.CompletionPopupAuto)
        self.token.completion.connect(self._findCompletion)
        self.token.completionObject().clear();
        self.token.returnPressed.connect(self.literalSearch)
        self.filter.returnPressed.connect(self.literalSearch)
        self.settings.clicked.connect(self.getSettings)
        self.tree.doubleClicked.connect(self.navigateTo)

    def __del__(self):
        """Plugins that use a toolview need to delete it for reloading to work."""
        if self.toolView:
            self.toolView.deleteLater()
            self.toolView = None

    @pyqtSlot()
    def literalSearch(self):
        """Lookup a single token and return the modelIndex of any definition."""
        definitionIndex = self.model.literalTokenSearch(self.toolView, self.token.text(), self.filter.text())
        self.tree.resizeColumnToContents(0)
        return definitionIndex

    def _findCompletion(self, token):
        """Fill the completion object with potential matches."""
        completionObj = self.token.completionObject()
        completionObj.clear()
        if len(token) < kate.configuration["keySize"]:
            #
            # Don't try to match if the token is too short.
            #
            return
        try:
            #
            # Add the first item if we find one, then any other matches.
            #
            lastToken = token
            lastOffset, lastName = self.dataSource.prefixSearchFirst(lastToken)
            while True:
                completionObj.addItem(lastName)
                lastOffset, lastName = self.dataSource.prefixSearchNext(lastOffset, lastToken)
        except IndexError:
            return

    @pyqtSlot("QModelIndex &")
    def navigateTo(self, index):
        """Jump to the selected entry.
        If the match entry is a filename, just jump to the file. If the entry
        if a specfic match, open the file and jump tothe location of the match.
        """
        parent = index.parent()
        if parent.isValid():
            #
            # We got a specific search result.
            #
            file = parent.data()
            line = int(index.sibling(index.row(), 1).data()) - 1
            column = index.sibling(index.row(), 2).data(Qt.UserRole + 1)
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

    @pyqtSlot()
    def getSettings(self):
        """Show the settings dialog.
        Establish our configuration. Loop until we have a viable settings, or
        the user cancels.
        """
        fileSet = False
        transformOK = False
        while not fileSet or not transformOK:
            fileSet = False
            transformOK = False
            dialog = ConfigDialog(kate.mainWindow().centralWidget())
            status = dialog.exec_()
            if status == QDialog.Rejected:
                break
            dialog.widget.apply()
            #
            # Check the save file name.
            #
            try:
                #
                # Only re-read the file if it has changed.
                #
                if not searchBar.dataSource.file or (searchBar.dataSource.file.name != kate.configuration["idFile"]):
                    searchBar.dataSource.setFile(kate.configuration["idFile"])
                fileSet = True
            except IOError as detail:
                KMessageBox.error(self.parent(), str(detail), i18n("ID database error"))
            #
            # Check the transformation settings.
            #
            try:
                transformationKey = kate.configuration["srcIn"]
                percentI = kate.configuration["srcOut"].find("%i")
                if len(transformationKey) and (percentI > -1):
                    #
                    # A transformation of the file name is with the output
                    # using the %i placeholder. Ensure the idFile is usable for
                    # that purpose.
                    #
                    insertLeft, discard = kate.configuration["idFile"].split(transformationKey, 1)
                transformOK = True
            except ValueError as detail:
                KMessageBox.error(self.parent(), i18n("'{}' does not contain '{}'").format(kate.configuration["idFile"], transformationKey), i18n("Cannot use %i"))
        return fileSet and transformOK

    def show(self):
        if not self.gotSettings:
            self.gotSettings = self.getSettings()
        if self.gotSettings:
            kate.mainInterfaceWindow().showToolView(self.toolView)
        return self.gotSettings

    def hide(self):
        kate.mainInterfaceWindow().hideToolView(self.toolView)

def wordAtCursorPosition(line, cursor):
    ''' Get the word under the active view's cursor in the given document.'''
    # Better to use word boundaries than to hardcode valid letters because
    # expansions should be able to be in any unicode character.
    wordBoundary = re.compile("\W", re.UNICODE)
    start = end = cursor.column()
    if start == len(line) or wordBoundary.match(line[start]):
        start -= 1
    while start >= 0 and not wordBoundary.match(line[start]):
        start -= 1
    start += 1
    while end < len(line) and not wordBoundary.match(line[end]):
        end += 1
    return start, end

def wordAtCursor(document, view):
    ''' Get the word under the active view's cursor in the given document.
    Stolen from the expand plugin!'''
    cursor = view.cursorPosition()
    line = document.line(cursor.line())
    start, end = wordAtCursorPosition(line, cursor)
    return line[start:end]

@kate.action("Browse Tokens", shortcut = "Alt+1", menu = "&Gid")
def show():
    # Make all our config is initialised.
    ConfigWidget().apply()
    global searchBar
    if searchBar is None:
        searchBar = SearchBar(kate.mainWindow())
    return searchBar.show()

@kate.action("Lookup Current Token", shortcut = "Alt+2", menu = "&Gid", icon = "edit-find")
def lookup():
    global searchBar
    if show():
        if kate.activeView().selection():
            selectedText = kate.activeView().selectionText()
        else:
            selectedText = wordAtCursor(kate.activeDocument(), kate.activeView())
        searchBar.token.setText(selectedText)
        return searchBar.literalSearch()
    return None

@kate.action("Go to Definition", shortcut = "Alt+3", menu = "&Gid", icon = "go-jump-definition")
def gotoDefinition():
    global searchBar
    definitionIndex = lookup();
    if definitionIndex:
        searchBar.navigateTo(definitionIndex)

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
