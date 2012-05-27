# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
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

set(sources plugin.cpp engine.cpp utilities.cpp)

kde4_add_ui_files(sources manager.ui)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

kde4_add_plugin(pateplugin ${sources})

target_link_libraries(pateplugin ${KDE4_KDEUI_LIBS} ${PYTHON_LIBRARY} kateinterfaces)

install(TARGETS pateplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES pate.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(DIRECTORY kate DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/pate)
