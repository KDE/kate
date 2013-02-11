# -*- coding: utf-8 -*-
# Copyright (c) 2013 by Pablo Martín <goinnn@gmail.com>
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

import kate
from PyKDE4.kdecore import KConfig, KConfigGroup


def get_session():
    main_window = kate.mainWindow()
    title = unicode(main_window.windowTitle())
    session = None
    if title and title != 'Kate' and ":" in title:
        session = title.split(":")[0]
        if session == 'file':
            session = None
    if session:
        return session
    return get_last_session()


def get_last_session():
    config = KConfig('katerc')
    kgroup = KConfigGroup(config, "General")
    session = kgroup.readEntry("Last Session")
    if session:
        session = unicode(session)
        session = session.replace('.katesession', '')
        return session
    return None
