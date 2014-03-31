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

macro_optional_find_package(PythonLibrary)
macro_log_feature(PYTHON_LIBRARY "PythonLibrary" "Python Library" "")

macro_optional_find_package(SIP)
macro_log_feature(SIP_FOUND "SIP" "Support for SIP Python bindings" "")

macro_optional_find_package(PyQt4)
macro_log_feature(PYQT4_FOUND "PyQt4" "Python bindings for Qt4" "")

macro_optional_find_package(PyKDE4)
macro_log_feature(PYKDE4_FOUND "PyKDE4" "Python bindings for KDE4" "")

if (NOT PYTHONLIBRARY_FOUND)
    message(WARNING "Pate plugin needs Python Library.")
    return()
endif (NOT PYTHONLIBRARY_FOUND)
if (NOT PYTHON_LIBRARY)
    message(WARNING "Pate plugin needs Python Library. (We are halfway there broken installation??")
    return()
endif (NOT PYTHON_LIBRARY)
if (${PYTHON_SHORT_VERSION} LESS 3.4)
    message(WARNING "Pate plugin needs Python version 3.4 or later. " ${PYTHON_SHORT_VERSION})
    return()
endif (${PYTHON_SHORT_VERSION} LESS 3.4)
if (NOT SIP_FOUND OR 0x${SIP_VERSION} LESS 0x40701)
    message(WARNING "Pate plugin needs SIP 4.7.1 or later. " ${SIP_VERSION})
    return()
endif (NOT SIP_FOUND OR 0x${SIP_VERSION} LESS 0x40701)
if (NOT SIP_EXECUTABLE OR NOT EXISTS ${SIP_EXECUTABLE})
    message(WARNING "Pate plugin needs SIP executable " ${SIP_EXECUTABLE})
    return()
endif (NOT SIP_EXECUTABLE OR NOT EXISTS ${SIP_EXECUTABLE})
if (NOT EXISTS ${PYQT4_SIP_DIR}/QtCore/QtCoremod.sip)
    message(WARNING "Pate plugin needs the PyQt4 development sip file QtCoremod.sip")
    return()
endif()
if (NOT EXISTS ${PYKDE4_SIP_DIR}/kdecore/kdecoremod.sip)
    message(WARNING "Pate plugin needs the PyKDE4 development sip files kdecoremod.sip")
    return()
endif()
if (NOT PYQT4_FOUND OR 0x${PYQT4_VERSION} LESS 0x040301)
    message(WARNING "Pate plugin needs PyQT 4.3.1 or later. " ${PYQT4_VERSION})
    return()
endif (NOT PYQT4_FOUND OR 0x${PYQT4_VERSION} LESS 0x040301)
if (NOT PYKDE4_FOUND)
    message(WARNING "Pate plugin needs PyKDE4.")
    return()
endif (NOT PYKDE4_FOUND)

#
# Use the Kate header files from the current source tree.
#
set(KATE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../../../kate)

include(KDE4Defaults)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${SIP_INCLUDE_DIR} ${PYTHON_INCLUDE_PATH}
    # Qt
    ${QT_INCLUDE_DIR} ${QT_QT_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTNETWORK_INCLUDE_DIR} ${QT_QTOPENGL_INCLUDE_DIR} ${QT_QTSQL_INCLUDE_DIR} ${QT_QTXML_INCLUDE_DIR} ${QT_QTSVG_INCLUDE_DIR} ${QT_WEBKIT_INCLUDE_DIR}
    # KDE
    ${KDE4_INCLUDE_DIR} ${KDE4_INCLUDE_DIR}/solid ${KDE4_INCLUDE_DIR}/kio ${KDE4_INCLUDE_DIR}/dom ${KDE4_INCLUDE_DIR}/ksettings
    # Kate
    ${KATE_INCLUDES} ${KATE_INCLUDES}/interfaces ${KATE_INCLUDES}/interfaces/kate)

add_subdirectory(sip)
add_subdirectory(src)
