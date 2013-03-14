# This file is part of Pate, Kate' Python scripting plugin.
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

include(SIPMacros)

set(SIP_INCLUDES ${PYQT4_SIP_DIR} ${PYKDE4_SIP_DIR} ./kate)
set(SIP_CONCAT_PARTS 1)
set(SIP_TAGS ALL WS_X11 ${PYQT4_VERSION_TAG})

if(PYQT4_VERSION STRLESS "040905")
    set(SIP_EXTRA_OPTIONS -g)
else (PYQT4_VERSION STRLESS "040905")
    # Disable QVector<int> for newer PyQt
    set(SIP_EXTRA_OPTIONS -g -x PyKDE_QVector)
endif(PYQT4_VERSION STRLESS "040905")

add_sip_python_module(PyKate4.kate ./kate/katemod.sip ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces)

install(FILES
    ./__init__.py
    DESTINATION ${PYTHON_SITE_PACKAGES_INSTALL_DIR}/PyKate4/)

