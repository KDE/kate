include(CMakeParseArguments)

#
# Simple helper function to install plugin and related files
# having only a name of the plugin...
# (just to reduce syntactic noise when a lot of plugins get installed)
#
function(install_pate_plugin name)
    set(_options)
    set(_one_value_args)
    set(_multi_value_args PATTERN FILE)
    cmake_parse_arguments(install_pate_plugin "${_options}" "${_one_value_args}" "${_multi_value_args}" ${ARGN})
    if(NOT name)
        message(FATAL_ERROR "Plugin filename is not given")
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}.py)
        install(FILES katepate_${name}.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        foreach(_f ${name}.py ${name}.ui ${name}_ui.rc ${install_pate_plugin_FILE})
            if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_f})
                install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${_f} DESTINATION ${DATA_INSTALL_DIR}/kate/pate)
            endif()
        endforeach()
    elseif(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${name})
        set(_patterns "")
        foreach(_p ${install_pate_plugin_PATTERN})
            set(_patterns ${_patterns} PATTERN ${_p})
        endforeach()
        install(FILES ${name}/katepate_${name}.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        install(
            DIRECTORY ${name}
            DESTINATION ${DATA_INSTALL_DIR}/kate/pate
            FILES_MATCHING
                PATTERN "*.py"
                PATTERN "*.ui"
                PATTERN "*_ui.rc"
                PATTERN "__pycache__/*" EXCLUDE
                ${_patterns}
          )
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
install_pate_plugin(python_console_ipython PATTERN "*.css")
install_pate_plugin(python_utils)
install_pate_plugin(try_open_file)
install_pate_plugin(xml_pretty)

if(PYTHON_VERSION_MAJOR VERSION_EQUAL 3)
    install_pate_plugin(cmake_utils)
    install_pate_plugin(js_utils PATTERN "*.json" PATTERN "*.js")
    install_pate_plugin(expand PATTERN "*.expand")
endif()

if(PYTHON_VERSION_MAJOR VERSION_EQUAL 2)
    install_pate_plugin(python_autocomplete)
endif()

install(
    DIRECTORY libkatepate
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING PATTERN "*.py"
  )
