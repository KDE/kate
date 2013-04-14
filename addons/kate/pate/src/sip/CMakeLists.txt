# This file is part of PyKDE4.
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

#
# The .sip files being built here are generated using MetaSIP from
# http://www.riverbankcomputing.com/hg/metasip. This is a relatively new tool
# and is typically not packaged by distros. A summary of the steps needed to
# get it working are at http://python.6.n6.nabble.com/ANN-MetaSIP-v1-0-Released-tp4715818p4991287.html.
# The result is a MetaSIP .msp project file, from which MetaSIP's CLI
# can be used to generate the .sip files. The MetaSIP GUI is used to create
# the .msp file:
#
# msip <whatever>.msp
#
# To recreate the .sip files, cd to this directory and then:
#
# msip -g . <whatever>.msp
#
# For now, the .msp file and the .sip files are checked into this directory,
# and the <whatever> subdirectory respectively.
#

include(SIPMacros)
include(KDE4Defaults)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    # Okteta
    /usr/include/okteta1)

set(SIP_INCLUDES ${PYQT4_SIP_DIR} ${PYKDE4_SIP_DIR} ./okteta)
set(SIP_CONCAT_PARTS 1)
set(SIP_TAGS ALL WS_X11 ${PYQT4_VERSION_TAG})

if(PYQT4_VERSION LESS "040905")
    set(SIP_EXTRA_OPTIONS -g)
else (PYQT4_VERSION LESS "040905")
    # Disable QVector<int> for newer PyQt
    set(SIP_EXTRA_OPTIONS -g -x PyKDE_QVector)
endif(PYQT4_VERSION LESS "040905")

add_sip_python_module(PyKate4.okteta ./okteta/oktetamod.sip ${KDE4_KDEUI_LIBS} okteta1core okteta1gui)

install(FILES
    ./__init__.py
    DESTINATION ${PYTHON_SITE_PACKAGES_INSTALL_DIR}/PyKate4/)
