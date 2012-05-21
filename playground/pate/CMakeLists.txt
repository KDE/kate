# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
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

project(pate)

find_package(KDE4 REQUIRED)
find_package(Qt4 REQUIRED)
find_package(PythonLibrary REQUIRED)

find_package(SIP REQUIRED)
if(SIP_VERSION STRLESS "040701")
    message(FATAL_ERROR "The version of SIP found is too old. 4.7.1 or later is needed.")
endif(SIP_VERSION STRLESS "040701")
include(SIPMacros)

find_package(PyQt4 REQUIRED)
if(PYQT4_VERSION STRLESS "040301")
    message(FATAL_ERROR "The version of PyQt found is too old. 4.3.1 or later is required.")
endif(PYQT4_VERSION STRLESS "040301")

find_package(PyKDE4 REQUIRED)

# set(CMAKE_VERBOSE_MAKEFILE ON)

#
# Use the Kate header files from the current source tree.
#
set(KATE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../../kate)

include(KDE4Defaults)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${SIP_INCLUDE_DIR} ${PYTHON_INCLUDE_PATH}
    # Qt
    ${QT_INCLUDE_DIR} ${QT_QT_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTNETWORK_INCLUDE_DIR} ${QT_QTOPENGL_INCLUDE_DIR} ${QT_QTSQL_INCLUDE_DIR} ${QT_QTXML_INCLUDE_DIR} ${QT_QTSVG_INCLUDE_DIR}
    # KDE
    ${KDE4_INCLUDE_DIR} ${KDE4_INCLUDE_DIR}/solid ${KDE4_INCLUDE_DIR}/kio ${KDE4_INCLUDE_DIR}/kdeprint ${KDE4_INCLUDE_DIR}/kdeprint/lpr ${KDE4_INCLUDE_DIR}/dom ${KDE4_INCLUDE_DIR}/ksettings
    # Kate
    ${KATE_INCLUDES} ${KATE_INCLUDES}/interfaces ${KATE_INCLUDES}/interfaces/kate)

# XX move this into a CMakeLists.txt for sip/. Gets boggling with things 
# looking for print_status.cmake in wrong places etc.
set(SIP_INCLUDES ${PYQT4_SIP_DIR} ${PYKDE4_SIP_DIR} sip/kate)
set(SIP_CONCAT_PARTS 1)
set(SIP_TAGS ALL WS_X11 ${PYQT4_VERSION_TAG})
set(SIP_EXTRA_OPTIONS -g)
add_sip_python_module(PyKDE4.kate sip/kate/katemod.sip ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces)


add_subdirectory(src)

