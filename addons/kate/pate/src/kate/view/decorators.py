# -*- coding: utf-8 -*-
#
# This file is part of Pate, Kate' Python scripting plugin.
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

from ..api import *
from ..decorators import viewCreated


class _SubscriberInfo:
    # TODO Turn to named tuples?
    def __init__(self, signal, receiver):
        self.signal = signal
        self.receiver = receiver


def _subscribe_for_events(subscribedPlugins, view):
    for plugin, subscribers in subscribedPlugins.items():
        assert(isinstance(subscribers, list))
        for subscriber in subscribers:
            kDebug('Subscribe {}/{} to receive {}'.format(
                plugin
              , subscriber.receiver.__name__
              , subscriber.signal
              ))
            view.connect(view, QtCore.SIGNAL(subscriber.signal), subscriber.receiver)


def _make_sure_subscribers_queue_exists(plugin, dst, queue):
    if not hasattr(dst, queue):
        setattr(dst, queue, dict())
    if not plugin in getattr(dst, queue):
        getattr(dst, queue)[plugin] = list()


@viewCreated
def _on_view_created(view):
    assert(view is not None)
    if hasattr(_on_view_created, 'view_event_subscribers'):
        assert(isinstance(_on_view_created.view_event_subscribers, dict))
        _subscribe_for_events(_on_view_created.view_event_subscribers, view)


def _queue_view_event_subscriber(signal, receiver):
    plugin = sys._getframe(2).f_globals['__name__']
    _make_sure_subscribers_queue_exists(plugin, _on_view_created, 'view_event_subscribers')
    _on_view_created.view_event_subscribers[plugin].append(_SubscriberInfo(signal, receiver))


#
# View events to subscribe from plugins
#


def cursorPositionChanged(receiver):
    _queue_view_event_subscriber(
        'cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)'
      , receiver
      )
    return receiver


def focusIn(receiver):
    _queue_view_event_subscriber('focusIn(KTextEditor::View*)', receiver)
    return receiver


def focusOut(receiver):
    _queue_view_event_subscriber('focusOut(KTextEditor::View*)', receiver)
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


def viewEditModeChanged(receiver):
    _queue_view_event_subscriber(
        'viewEditModeChanged(KTextEditor::View*, KTextEditor::View::EditMode)'
      , receiver
      )
    return receiver


def viewModeChanged(receiver):
    _queue_view_event_subscriber('viewModeChanged(KTextEditor::View*)', receiver)
    return receiver

