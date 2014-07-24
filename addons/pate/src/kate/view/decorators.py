# -*- coding: utf-8 -*-
# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2013 Alex Turbov <i.zaufi@gmail.com>
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

'''Decorators used in plugins to handle view events'''

import sys

from PyQt4 import QtCore

from ..api import qDebug
from ..document_view_helpers import _make_sure_subscribers_queue_exists, _on_view_created, _SubscriberInfo


def _queue_view_event_subscriber(signal, receiver):
    '''Helper function to register a new handler'''
    plugin = sys._getframe(2).f_globals['__name__']
    _make_sure_subscribers_queue_exists(plugin, _on_view_created, 'view_event_subscribers')
    _on_view_created.view_event_subscribers[plugin].append(_SubscriberInfo(signal, receiver))


#
# View events to subscribe from plugins
#
# http://api.kde.org/4.x-api/kdelibs-apidocs/interfaces/ktexteditor/html/classKTextEditor_1_1View.html
#

def contextMenuAboutToShow(receiver):
    _queue_view_event_subscriber(
        'contextMenuAboutToShow(KTextEditor::View*, QMenu*)'
      , receiver
      )
    return receiver


def cursorPositionChanged(receiver):
    _queue_view_event_subscriber(
        'cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)'
      , receiver
      )
    return receiver


def focusIn(receiver):
    _queue_view_event_subscriber(
        'focusIn(KTextEditor::View*)'
      , receiver
      )
    return receiver


def focusOut(receiver):
    _queue_view_event_subscriber(
        'focusOut(KTextEditor::View*)'
      , receiver
      )
    return receiver


def horizontalScrollPositionChanged(receiver):
    _queue_view_event_subscriber(
        'horizontalScrollPositionChanged(KTextEditor::View*)'
      , receiver
      )
    return receiver


def informationMessage(receiver):
    _queue_view_event_subscriber(
        'informationMessage(KTextEditor::View*, const QString&)'
      , receiver
      )
    return receiver


def mousePositionChanged(receiver):
    _queue_view_event_subscriber(
        'mousePositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)'
      , receiver
      )
    return receiver


def selectionChanged(receiver):
    _queue_view_event_subscriber('selectionChanged(KTextEditor::View *)', receiver)
    return receiver


def textInserted(receiver):
    _queue_view_event_subscriber(
        'textInserted(KTextEditor::View*, const KTextEditor::Cursor&, const QString&)'
      , receiver
      )
    return receiver


def verticalScrollPositionChanged(receiver):
    _queue_view_event_subscriber(
        'verticalScrollPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)'
      , receiver
      )
    return receiver


def viewInputModeChanged(receiver):
    _queue_view_event_subscriber(
        'viewInputModeChanged(KTextEditor::View*, KTextEditor::View::InputMode)'
      , receiver
      )
    return receiver


def viewModeChanged(receiver):
    _queue_view_event_subscriber('viewModeChanged(KTextEditor::View*, KTextEditor::View::ViewMode)', receiver)
    return receiver
