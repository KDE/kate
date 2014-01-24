# -*- coding: utf-8 -*-
# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
# Copyright (C) 2013 Shaheed Haque <srhaque@theiet.org>
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

'''Shared code for view/document decorators'''

from PyQt4 import QtCore

from .api import qDebug
from .decorators import viewCreated


class _SubscriberInfo:
    # TODO Turn to named tuples?
    def __init__(self, signal, receiver):
        self.signal = signal
        self.receiver = receiver


def _subscribe_for_events(subscribedPlugins, source):
    for plugin, subscribers in subscribedPlugins.items():
        assert(isinstance(subscribers, list))
        for subscriber in subscribers:
            qDebug('Subscribe {}/{} to receive {}'.format(
                plugin
              , subscriber.receiver.__name__
              , subscriber.signal
              ))
            source.connect(source, QtCore.SIGNAL(subscriber.signal), subscriber.receiver)


def _make_sure_subscribers_queue_exists(plugin, dst, queue):
    if not hasattr(dst, queue):
        setattr(dst, queue, dict())
    if not plugin in getattr(dst, queue):
        getattr(dst, queue)[plugin] = list()


@viewCreated
def _on_view_created(view):
    assert(view is not None)

    # Subscribe registered handlers for view events
    if hasattr(_on_view_created, 'view_event_subscribers'):
        assert(isinstance(_on_view_created.view_event_subscribers, dict))
        _subscribe_for_events(_on_view_created.view_event_subscribers, view)

    # Subscribe registered handlers for document events
    if hasattr(_on_view_created, 'document_event_subscribers'):
        assert(isinstance(_on_view_created.document_event_subscribers, dict))
        _subscribe_for_events(_on_view_created.document_event_subscribers, view.document())
