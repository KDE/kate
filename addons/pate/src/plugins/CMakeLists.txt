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

include(CMakeParseArguments)

#
# Simple helper function to install plugin and related files
# having only a name of the plugin...
# (just to reduce syntactic noise when a lot of plugins get installed)
#
function(install_pate_plugin name)
    set(_options)
    set(_one_value_args)
    set(_multi_value_args PATTERNS FILE)
    cmake_parse_arguments(install_pate_plugin "${_options}" "${_one_value_args}" "${_multi_value_args}" ${ARGN})
    if(NOT name)
        message(FATAL_ERROR "Plugin filename is not given")
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py)
        install(FILES katepate_${name}.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        foreach(_f ${name}.py ${name}.ui ${install_pate_plugin_FILE})
            if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_f})
                install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${_f} DESTINATION ${DATA_INSTALL_DIR}/kate/pate)
            endif()
        endforeach()
        foreach(_f ${name}_ui.rc ${install_pate_plugin_FILE})
            if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_f})
                install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${_f} DESTINATION ${KXMLGUI_INSTALL_DIR}/kate/pate)
            endif()
        endforeach()
    elseif(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${name})
        install(FILES ${name}/katepate_${name}.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        install(
            DIRECTORY ${name}
            DESTINATION ${DATA_INSTALL_DIR}/kate/pate
            FILES_MATCHING
                PATTERN "*.py"
                PATTERN "*.ui"
                PATTERN "__pycache__*" EXCLUDE
           )
        install(
            DIRECTORY ${name}
            DESTINATION ${KXMLGUI_INSTALL_DIR}/kate/pate
            FILES_MATCHING
                PATTERN "*_ui.rc"
                PATTERN "__pycache__*" EXCLUDE
           )
        # TODO Is there any way to form a long PATTERN options string
        # and use it in a single install() call?
        # NOTE Install specified patterns one-by-one...
        foreach(_pattern ${install_pate_plugin_PATTERNS})
            install(
                DIRECTORY ${name}
                DESTINATION ${DATA_INSTALL_DIR}/kate/pate
                FILES_MATCHING
                    PATTERN "${_pattern}"
                    PATTERN "__pycache__*" EXCLUDE
              )
        endforeach()
    else()
        message(FATAL_ERROR "Do not know what to do with ${name}")
    endif()
endfunction()

# TODO The following plugins must be ported to new style...
# install_pate_plugin(django_utils)
# install_pate_plugin(gdb)

install_pate_plugin(block)
install_pate_plugin(color_tools FILE color_tools_toolview.ui)
install_pate_plugin(commentar)
install_pate_plugin(format)
install_pate_plugin(gid)
install_pate_plugin(python_autocomplete_jedi)
install_pate_plugin(python_console_classic)
install_pate_plugin(python_console_ipython PATTERNS "*.css")
install_pate_plugin(python_utils)
install_pate_plugin(try_open_file)
install_pate_plugin(xml_pretty)

if(PYTHON_VERSION_MAJOR VERSION_EQUAL 3)
    install_pate_plugin(cmake_utils)
    install_pate_plugin(js_utils PATTERNS "*.json")
    install_pate_plugin(expand PATTERNS "*.expand" "templates/*.tpl")
endif()

if(PYTHON_VERSION_MAJOR VERSION_EQUAL 2)
    install_pate_plugin(python_autocomplete)
endif()

install(
    DIRECTORY libkatepate
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING
        PATTERN "*.py"
        PATTERN "__pycache__*" EXCLUDE
  )
