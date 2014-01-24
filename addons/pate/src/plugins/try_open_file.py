# -*- coding: utf-8 -*-
#
# Try to open selected text as URI to a document
# Copyright 2013 by Alex Turbov <i.zaufi@gmail.com>
#
#
# This software is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this software.  If not, see <http://www.gnu.org/licenses/>.
#

from PyKDE4.kdecore import i18nc, KUrl
from PyKDE4.ktexteditor import KTextEditor
from PyKDE4.kdeui import KXMLGUIClient

import kate
import kate.view

# TODO Make it configurable?
_SANE_URI_LENGTH = 300
_URI_LENGTH_SHOW_THRESHOLD = 30


def _try_make_url_from_text(text):
    # TODO How about to analyze the text given and reject definitely not an URI?
    return ''.join([line.strip() for line in text.split('\n')])


def _try_open_show_error(uri):
    assert('Sanity check' and uri is not None)
    view = kate.mainInterfaceWindow().openUrl(uri)
    if view is None:
        kate.ui.popup(
            i18nc('@title:window', 'Error')
          , i18nc(
                '@info:tooltip'
              , 'Unable to open the selected document: <filename>%1</filename>'
              , str(uri)
              )
          , 'dialog-error'
          )


@kate.action
def tryOpen():
    view = kate.activeView()
    assert('View expected to be valid' and view is not None)
    assert('This action supposed to select some text before' and view.selection())

    doc = view.document()
    doc_url = doc.url()
    new_url = KUrl(_try_make_url_from_text(view.selectionText()))

    kate.qDebug('Current document URI: {}'.format(repr(doc_url)))

    # 0) Make sure it is valid
    if not new_url.isValid():
        kate.ui.popup(
            i18nc('@title:window', 'Error')
          , i18nc('@info:tooltip', "Selected text doesn't looks like a valid URI")
          , 'dialog-error'
          )
        return

    # 1) Is it have some schema? and current document is not 'Untitled'...
    if new_url.isRelative() and not doc_url.isEmpty():
        # Replace filename from the doc_url w/ the current selection
        new_url = doc_url.upUrl()
        new_url.addPath(view.selectionText())

    kate.qDebug('Trying URI: {}'.format(repr(new_url)))
    # Ok, try to open it finally
    _try_open_show_error(new_url)


@kate.view.selectionChanged
def toggleSelectionSensitiveActions(view):
    clnt = kate.getXmlGuiClient()
    filename = _try_make_url_from_text(view.selectionText())
    kate.qDebug('Original new filename: {}'.format(repr(filename)))

    if view.selection() and len(filename) < _SANE_URI_LENGTH:
        clnt.stateChanged('has_selection')
        # Change action text...
        if _URI_LENGTH_SHOW_THRESHOLD < len(filename):
            lead_pos = int(2 * _URI_LENGTH_SHOW_THRESHOLD / 3)
            tail_pos = -int(_URI_LENGTH_SHOW_THRESHOLD / 3)
            filename = filename[:lead_pos] + '...' + filename[tail_pos:]
        assert('Sanity check' and hasattr(tryOpen, 'action'))
        kate.qDebug('New filename: {}'.format(filename))
        tryOpen.action.setText(
            i18nc('@ation:inmenu', 'Open <filename>%1</filename>', filename)
          )
    else:
        clnt.stateChanged('has_selection', KXMLGUIClient.StateReverse)
        tryOpen.action.setText(i18nc('@ation:inmenu', 'Open selected document'))
