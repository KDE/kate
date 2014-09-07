# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
# Copyright (C) 2012, 2013 Shaheed Haque <srhaque@theiet.org>
# Copyright (C) 2013 Alex Turbov <i.zaufi@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

install(TARGETS katepateplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
install(FILES katepythonplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR})
install(FILES pate.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/kate/plugins/katepateplugin)

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
