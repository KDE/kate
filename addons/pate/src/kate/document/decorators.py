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

'''Decorators used in plugins to handle document events'''

import sys

from PyQt4 import QtCore

from ..api import qDebug
from ..document_view_helpers import _make_sure_subscribers_queue_exists, _on_view_created, _SubscriberInfo


def _queue_document_event_subscriber(signal, receiver):
    '''Helper function to register a new handler'''
    plugin = sys._getframe(2).f_globals['__name__']
    _make_sure_subscribers_queue_exists(plugin, _on_view_created, 'document_event_subscribers')
    _on_view_created.document_event_subscribers[plugin].append(_SubscriberInfo(signal, receiver))


#
# Document events to subscribe from plugins
#
# http://api.kde.org/4.x-api/kdelibs-apidocs/interfaces/ktexteditor/html/classKTextEditor_1_1Document.html
#
# TODO Add more inherited signals?
#

def aboutToClose(receiver):
    _queue_document_event_subscriber(
        'aboutToClose(KTextEditor::Document*)'
      , receiver
      )
    return receiver


def aboutToReload(receiver):
    _queue_document_event_subscriber(
        'aboutToReload(KTextEditor::Document*)'
      , receiver
      )
    return receiver


def documentNameChanged(receiver):
    _queue_document_event_subscriber(
        'documentNameChanged(KTextEditor::Document*)'
      , receiver
      )
    return receiver


def documentSavedOrUploaded(receiver):
    _queue_document_event_subscriber(
        'documentSavedOrUploaded(KTextEditor::Document*, bool)'
      , receiver
      )
    return receiver


def documentUrlChanged(receiver):
    _queue_document_event_subscriber(
        'documentUrlChanged(KTextEditor::Document*)'
      , receiver
      )
    return receiver


def exclusiveEditEnd(receiver):
    _queue_document_event_subscriber(
        'exclusiveEditEnd(KTextEditor::Document*)'
      , receiver
      )
    return receiver


def exclusiveEditStart(receiver):
    _queue_document_event_subscriber(
        'exclusiveEditStart(KTextEditor::Document*)'
      , receiver
      )
    return receiver


def highlightingModeChanged(receiver):
    _queue_document_event_subscriber(
        'highlightingModeChanged(KTextEditor::Document*)'
      , receiver
      )
    return receiver


def modeChanged(receiver):
    _queue_document_event_subscriber(
        'modeChanged(KTextEditor::Document*)'
      , receiver
      )
    return receiver


def modifiedChanged(receiver):
    _queue_document_event_subscriber(
        'modifiedChanged(KTextEditor::Document*)'
      , receiver
      )
    return receiver


def reloaded(receiver):
    _queue_document_event_subscriber(
        'reloaded(KTextEditor::Document*)'
      , receiver
      )
    return receiver


def textChanged(receiver):
    _queue_document_event_subscriber(
        'textChanged(KTextEditor::Document*, const KTextEditor::Range&, const QString&, const KTextEditor::Range&)'
      , receiver
      )
    return receiver


def textInserted(receiver):
    _queue_document_event_subscriber(
        'textInserted(KTextEditor::Document*, const KTextEditor::Range&)'
      , receiver
      )
    return receiver


def textRemoved(receiver):
    _queue_document_event_subscriber(
        'textRemoved(KTextEditor::Document*, const KTextEditor::Range&, const QString&)'
      , receiver
      )
    return receiver


def viewCreated(receiver):
    _queue_document_event_subscriber(
        'viewCreated(KTextEditor::Document*, KTextEditor::View*)'
      , receiver
      )
    return receiver
