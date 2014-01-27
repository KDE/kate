"""Browse the tokens in a GNU idutils ID file, and use it to navigate within a codebase.

The necessary parts of the ID file are held in memory to allow sufficient performance
for token completion, when looking up the usage of a token, or jumping to the
definition. etags(1) is used to locate the definition.
"""
#
# Copyright 2012, 2013, Shaheed Haque <srhaque@theiet.org>.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import kate

from PyQt4 import uic
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyKDE4.kdecore import *
from PyKDE4.kdeui import *
from PyKDE4.ktexteditor import KTextEditor

from .idutils import Lookup

import codecs
import os.path
import re
import sip
import subprocess
import sys
import time

idDatabase = None
searchBar = None
completionModel = None

def toStr(_bytes):
    if sys.version_info.major >= 3:
        return _bytes.decode("utf-8")
    else:
        return unicode(_bytes)

class ConfigWidget(QWidget):
    """Configuration widget for this plugin."""
    def __init__(self, parent = None, name = None):
        super(ConfigWidget, self).__init__(parent)
        #
        # Location of ID file.
        #
        self.idFile = None
        #
        # Completion string minimum size.
        #
        self.keySize = None
        #
        # Original file prefix.
        #
        self.srcIn = None
        #
        # Replacement file prefix.
        #
        self.srcOut = None

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
            try:
                insertLeft, discard = kate.configuration["idFile"].split(transformationKey, 1)
            except ValueError:
                #
                # No transformation is possible.
                #
                return file
            discard, insertRight = kate.configuration["srcOut"].split("%{idPrefix}", 1)
            file = insertLeft + insertRight + right
        else:
            file = kate.configuration["srcOut"] + right
    return file

class HistoryModel(QStandardItemModel):
    """Support the display of a stack of navigation points.
    Each visible row comprises { basename(fileName);line, text }.
    On column 0, we show a tooltip with the full fileName, and any icon.
    On column 1, we store line and column.
    """
    def __init__(self):
        """Constructor.
        """
        super(HistoryModel, self).__init__()
        self.setHorizontalHeaderLabels((i18n("File"), i18n("Match")))

    def add(self, fileName, icon, text, line, column, fileAndLine):
        """Add a new entry to the top of the stack."""
        #
        # Ignore if the top of the stack has an identical entry.
        #
        column0 = self.invisibleRootItem().child(0, 0)
        if column0:
            if fileName == column0.data(Qt.ToolTipRole):
                column1 = self.invisibleRootItem().child(0, 1)
                if line == column1.data(Qt.UserRole + 1):
                    return column0.index()
        column0 = QStandardItem(fileAndLine)
        if icon:
            column0.setIcon(KIcon(icon))
        column0.setData(fileName, Qt.ToolTipRole)
        column1 = QStandardItem(text)
        column1.setData(line, Qt.UserRole + 1)
        column1.setData(column, Qt.UserRole + 2)
        resultRow = (column0, column1)
        self.invisibleRootItem().insertRow(0, resultRow)
        if self.rowCount() > 64:
            self.setRowCount(64)
        return resultRow[0].index()

    def read(self, index):
        """Extract a row."""
        row = index.row()
        column0 = index.sibling(row, 0)
        fileAndLine = column0.data()
        fileName = column0.data(Qt.ToolTipRole)
        icon = column0.data(Qt.DecorationRole)
        column1 = index.sibling(row, 1)
        text = column1.data()
        line = column1.data(Qt.UserRole + 1)
        column = column1.data(Qt.UserRole + 2)
        return (fileName, icon, text, line, column, fileAndLine)

class MatchesModel(HistoryModel):
    """Support the display of a list of entries matching a token.
    The display matches HistoryModel, but is a list not a stack.
    """

    _boredomInterval = 20

    def __init__(self, dataSource):
        """Constructor.

        @param dataSource    an instance of a Lookup().
        """
        super(MatchesModel, self).__init__()
        self.dataSource = dataSource

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
            lineNumberEnd = toStr(etagBytes).find(",", lineNumberStart)
            return int(etagBytes[lineNumberStart:lineNumberEnd]) - 1
        if etagBytes.startswith(bytearray(etagsCmd[0], "latin-1")):
            #
            # An error message was printed starting with "etags".
            #
            raise IOError(toStr(etagBytes))
        return None

    def _scanFile(self, regexp, filterRe, token, fileName, isDeclaration):
        """Scan a file looking for interesting hits. Return the QModelIndex of the last of any definitions we find."""
        definitionIndex = None
        fileRow = None
        hits = 0
        line = 0
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
                    hits += 1
                    column = match.start()
                    if line == definitionLine:
                        #
                        # Mark the line and the file as being a definition.
                        #
                        definitionIndex = self.add(fileName, "go-jump-definition", text[:-1], line, column)
                    elif isDeclaration:
                        self.add(fileName, "text-x-chdr", text[:-1], line, column)
                    else:
                        self.add(fileName, None, text[:-1], line, column)
                #
                # FF and VT are line endings it seems...
                #
                if text[-1] != '\f' and text[-1] != '\v':
                    line += 1
            if not hits:
                #
                # This was in the index, but we found no hits. Assuming the file
                # content has changed, we still permit navigation to the top of
                # the file.
                #
                self.add(fileName, "task-reject", "", 0, 0)
        except IOError as e:
            self.add(fileName, "face-sad", str(e), None, None)
        return definitionIndex

    def add(self, fileName, icon, text, line, column):
        """Append a new entry."""
        if line:
            column0 = QStandardItem("{}:{}".format(QFileInfo(fileName).fileName(), line + 1))
        else:
            column0 = QStandardItem(QFileInfo(fileName).fileName())
        if icon:
            column0.setIcon(KIcon(icon))
        column0.setData(fileName, Qt.ToolTipRole)
        column1 = QStandardItem(text)
        column1.setData(line, Qt.UserRole + 1)
        column1.setData(column, Qt.UserRole + 2)
        resultRow = (column0, column1)
        self.invisibleRootItem().appendRow(resultRow)
        return resultRow[0].index()

    def literalTokenSearch(self, parent, token, filter):
        """Add the entries which match the token to the matches, and return the QModelIndex of the last of any definitions we find.

        Entries are grouped under the file in which the hits are searched. Each
        entry shows the matched text, the line and column of the match. If so
        enabled, entries which are defintions according to etags are highlighted.

        If the output takes a long time to generate, the user is given options
        to continue or abort.
        """
        self.setRowCount(0)
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
                KMessageBox.error(parent.parent(), i18n("Filter '%1' is not a valid regular expression", filter), i18n("Invalid filter"))
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
        previousBoredomQuery = startBoredomQuery - MatchesModel._boredomInterval // 2
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
            newDefinitionIndex = self._scanFile(regexp, filterRe, token, fileName, isDeclaration)
            if newDefinitionIndex:
                definitionIndex = newDefinitionIndex
            filesListed += 1
            #
            # Time to query the user's boredom level?
            #
            if time.time() - previousBoredomQuery > MatchesModel._boredomInterval:
                r = KMessageBox.questionYesNoCancel(parent.parent(), i18n("Scanned %1 of %2 files in %3 seconds", filesListed, len(files), int(time.time() - startBoredomQuery)),
                        i18n("Scan more files?"), KGuiItem(i18n("All Files")), KGuiItem(i18n("More Files")), KStandardGuiItem.cancel())
                if r == KMessageBox.Yes:
                    previousBoredomQuery = time.time() + 10 * MatchesModel._boredomInterval
                elif r == KMessageBox.No:
                    previousBoredomQuery = time.time()
                else:
                    break
        #
        # Return the model index of the match.
        #
        return definitionIndex

class CompletionModel(KTextEditor.CodeCompletionModel):
    """Support Kate code completion.
    """
    def __init__(self, parent, dataSource):
        """Constructor.

        @param parent        Parent QObject.
        @param dataSource    An instance of a Lookup().
        """
        super(CompletionModel, self).__init__(parent)
        self.dataSource = dataSource
        self.completionObj = None

    def completionInvoked(self, view, range, invocationType):
        """Find matches for the range given.
        """
        token = view.document().text(range)
        if len(token) < kate.configuration["keySize"]:
            #
            # Don't try to match if the token is too short.
            #
            self.setRowCount(0)
            return
        self.completionObj = list()
        try:
            #
            # Add the first item if we find one, then any other matches.
            #
            lastToken = token
            lastOffset, lastName = self.dataSource.prefixSearchFirst(lastToken)
            while True:
                self.completionObj.append(lastName)
                lastOffset, lastName = self.dataSource.prefixSearchNext(lastOffset, lastToken)
        except IndexError:
            pass
        self.setRowCount(len(self.completionObj))

    def columnCount(self, index):
        return KTextEditor.CodeCompletionModel.Name

    def data(self, index, role):
        """Return the data defining the item.
        """
        column = index.column()
        if column == KTextEditor.CodeCompletionModel.Name:
            if role == Qt.DisplayRole:
                # The match itself.
                return self.completionObj[index.row()]
            elif role ==  KTextEditor.CodeCompletionModel.HighlightingMethod:
                # Default highlighting.
                QVariant.Invalid
            else:
                #print "data()", index.row(), "Name", role
                return None
        else:
            #print "data()", index.row(), column, role
            return None

class SearchBar(QObject):
    def __init__(self, dataSource):
        super(SearchBar, self).__init__(None)
        self.lastToken = None
        self.lastOffset = None
        self.lastName = None
        self.gotSettings = False
        self.dataSource = dataSource
        self.toolView = kate.mainInterfaceWindow().createToolView("idutils_gid_plugin", kate.Kate.MainWindow.Bottom, SmallIcon("edit-find"), i18n("gid Search"))
        # By default, the toolview has box layout, which is not easy to delete.
        # For now, just add an extra widget.
        top = QWidget(self.toolView)

        # Set up the user interface from Designer.
        uic.loadUi(os.path.join(os.path.dirname(__file__), "tool.ui"), top)
        self.token = top.token
        self.filter = top.filter
        self.settings = top.settings

        self.matchesModel = MatchesModel(self.dataSource)
        self.matchesWidget = top.matches
        self.matchesWidget.verticalHeader().setDefaultSectionSize(20)
        self.matchesWidget.setModel(self.matchesModel)
        self.matchesWidget.setColumnWidth(0, 200)
        self.matchesWidget.setColumnWidth(1, 400)
        self.matchesWidget.activated.connect(self.navigateToMatch)

        self.historyModel = HistoryModel()
        self.historyWidget = top.history
        self.historyWidget.verticalHeader().setDefaultSectionSize(20)
        self.historyWidget.setModel(self.historyModel)
        self.historyWidget.setColumnWidth(0, 200)
        self.historyWidget.setColumnWidth(1, 400)
        self.historyWidget.activated.connect(self.navigateToHistory)

        self.token.setCompletionMode(KGlobalSettings.CompletionPopupAuto)
        self.token.completion.connect(self._findCompletion)
        self.token.completionObject().clear();
        self.token.returnPressed.connect(self.literalSearch)
        self.filter.returnPressed.connect(self.literalSearch)
        self.settings.clicked.connect(self.getSettings)

    def __del__(self):
        """Plugins that use a toolview need to delete it for reloading to work."""
        assert(self.toolView is not None)
        mw = kate.mainInterfaceWindow()
        if mw:
            self.hide()
            mw.destroyToolView(self.toolView)
        self.toolView = None

    @pyqtSlot()
    def literalSearch(self):
        """Lookup a single token and return the modelIndex of any definition."""
        definitionIndex = self.matchesModel.literalTokenSearch(self.toolView, self.token.currentText(), self.filter.currentText())
        self.matchesWidget.resizeColumnsToContents()
        if definitionIndex:
            #
            # Set the navigation starting point to (the last of) any
            # definition we found.
            #
            self.matchesWidget.setCurrentIndex(definitionIndex)
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
    def navigateToMatch(self, index):
        """Jump to the selected entry."""
        (fileName, icon, text, nextLine, column, fileAndLine) = self.matchesModel.read(index)
        if not nextLine:
            return
        #
        # Add a history record for the current point.
        #
        cursor = kate.activeView().cursorPosition()
        currentLine = cursor.line()
        document = kate.activeDocument()
        self.historyModel.add(document.url().toLocalFile(), "arrow-right", document.line(currentLine), currentLine, cursor.column(), "{}:{}".format(document.documentName(), currentLine + 1))
        #
        # Navigate to the point in the file.
        #
        document = kate.documentManager.openUrl(KUrl.fromPath(fileName))
        kate.mainInterfaceWindow().activateView(document)
        point = KTextEditor.Cursor(nextLine, column)
        kate.activeView().setCursorPosition(point)
        #
        # Add this new point to the history.
        #
        self.historyModel.add(fileName, icon, text, nextLine, column, fileAndLine)
        self.historyWidget.resizeColumnsToContents()

    @pyqtSlot("QModelIndex &")
    def navigateToHistory(self, index):
        """Jump to the selected entry."""
        (fileName, icon, text, line, column, fileAndLine) = self.historyModel.read(index)
        #
        # Navigate to the original point in the file.
        #
        document = kate.documentManager.openUrl(KUrl.fromPath(fileName))
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
                KMessageBox.error(self.parent(), toStr(detail), i18n("ID database error"))
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
                # xgettext: no-python-format
                KMessageBox.error(self.parent(), i18n("'%1' does not contain '%2'", kate.configuration["idFile"], transformationKey), i18n("Cannot use %i"))
        return fileSet and transformOK

    def show(self):
        if not self.gotSettings:
            self.gotSettings = self.getSettings()
        if self.gotSettings:
            kate.mainInterfaceWindow().showToolView(self.toolView)
            self.token.setFocus(Qt.PopupFocusReason)
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

@kate.action
def show():
    """Browse the tokens in the ID file."""
    # Make all our config is initialised.
    ConfigWidget().apply()
    global idDatabase
    global searchBar
    if searchBar is None:
        idDatabase = Lookup()
        searchBar = SearchBar(idDatabase)
    global completionModel
    if completionModel is None:
        completionModel = CompletionModel(kate.mainWindow(), idDatabase)
    viewChanged()
    return searchBar.show()

@kate.action
def lookup():
    """Lookup the currently selected token.
    Find the token, filter the results.
    """
    global searchBar
    if show():
        if kate.activeView().selection():
            selectedText = kate.activeView().selectionText()
        else:
            selectedText = wordAtCursor(kate.activeDocument(), kate.activeView())
        searchBar.token.insertItem(0, selectedText)
        searchBar.token.setCurrentIndex(1)
        searchBar.token.setEditText(selectedText)
        return searchBar.literalSearch()
    return None

@kate.action
def gotoDefinition():
    """Go to the definition of the currently selected token.
    Find the token, search for definitions using etags, jump to the definition.
    """
    global searchBar
    definitionIndex = lookup()
    if definitionIndex:
        searchBar.navigateToMatch(definitionIndex)

@kate.configPage(
    i18nc("@item:inmenu", "GNU idutils")
  , i18nc("@title:group", "<command section='1'>gid</command> token Lookup and Navigation")
  , icon = "edit-find"
  )
def configPage(parent = None, name = None):
    return ConfigPage(parent, name)

@kate.unload
def destroy():
    """Plugins that use a toolview need to delete it for reloading to work."""
    global searchBar
    if searchBar:
        del searchBar
        #searchBar = None
    global completionModel
    if completionModel:
        del completionModel
        #completionModel = None

@kate.viewChanged
def viewChanged():
    ''' Calls the function when the view changes. To access the new active view,
    use kate.activeView() '''
    global completionModel
    if completionModel:
        view = kate.activeView()
        if view:
            completionInterface = view.codeCompletionInterface()
            completionInterface.registerCompletionModel(completionModel)
