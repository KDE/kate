#
# Install .desktop files for individual plugins
#
install(
    FILES
        cmake_utils/katepate_cmake_utils.desktop
        gid/katepate_gid.desktop
        katepate_block.desktop
        katepate_color_tools.desktop
        katepate_commentar.desktop
        katepate_expand.desktop
        katepate_format.desktop
        katepate_python_autocomplete_jedi.desktop
        katepate_xml_pretty.desktop
        python_autocomplete/katepate_python_autocomplete.desktop
        python_console_classic/katepate_python_console_classic.desktop
        python_console_ipython/katepate_python_console_ipython.desktop
        js_utils/katepate_js_utils.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}
  )

#
# Install individual Pate plugins.
#
install(
    FILES
        block.py block_ui.rc
        color_tools.py color_tools_ui.rc color_tools_toolview.ui
        commentar.py commentar_config.ui commentar_ui.rc
        expand.py expand_ui.rc
        format.py format_ui.rc
        python_autocomplete_jedi.py
        xml_pretty.py xml_pretty.ui xml_pretty_ui.rc
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
  )

#
# Install various directories w/ plugins
#
install(
    DIRECTORY
        django_utils
        expand
        gid
        libkatepate
        python_console_classic
        python_utils
        python_autocomplete
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING PATTERN "*.py" PATTERN "*.ui" PATTERN "*.rc"
  )

install(
    DIRECTORY expand
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING PATTERN "*.expand"
  )

install(
    DIRECTORY js_utils
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING PATTERN "*.py" PATTERN "*.ui" PATTERN "*.json" PATTERN "*.js" PATTERN "*.rc"
    )

install(
    DIRECTORY python_console_ipython
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING PATTERN "*.py" PATTERN "*.ui" PATTERN "*.css"
  )

install(
    DIRECTORY cmake_utils
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING PATTERN "*.py" PATTERN "*.ui" PATTERN "*.rc"
  )

# kate: indent-width 4;
