# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
# Copyright (C) 2012 Shaheed Haque <srhaque@theiet.org>
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

# by default only QtCore and QtGui modules are enabled
# other modules must be enabled like this:

set(QT_USE_QTWEBKIT TRUE)
set(sources engine.cpp plugin.cpp utilities.cpp )

# NOTE Disable trivial Qt keywords due conflicts w/ some Python.h header
# (at least version 3.3 of it has a member PyType_Spec::slots)
add_definitions(-DQT_NO_KEYWORDS)

kde4_add_ui_files(sources manager.ui)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

kde4_add_plugin(katepateplugin ${sources})

target_link_libraries(
    katepateplugin
    ${KDE4_KDEUI_LIBS}
    ${PYTHON_LIBRARY}
    ${QT_QTWEBKIT_LIBRARY}
    kateinterfaces
  )

install(TARGETS katepateplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES katepythonplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR})
install(FILES pate.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/katepateplugin)

# Install "built-in" plugins
install(
    DIRECTORY kate
    DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/pate
    FILES_MATCHING PATTERN "*.py"
  )

# Dive into plugins crowd
add_subdirectory(plugins)
add_subdirectory(test)

# kate: indent-width 4;
