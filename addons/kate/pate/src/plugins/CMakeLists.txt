#
# Install .desktop files for indivudual plugins
#
install(
    FILES
        cmake_utils/katepate_cmake_utils.desktop
        katepate_block.desktop
        katepate_color_tools.desktop
        katepate_commentar.desktop
        katepate_expand.desktop
        katepate_format.desktop
        python_autocomplete/katepate_python_autocomplete.desktop
        python_console_classic/katepate_python_console_classic.desktop
        python_console_ipython/katepate_python_console_ipython.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}
  )

#
# Install individual Pate plugins.
#
install(
    FILES
        block.py
        color_tools.py
        color_tools_toolview.ui
        commentar.py
        commentar_config.ui
        expand.py
        format.py
        xml_pretty.py
        xml_pretty.ui
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
    FILES_MATCHING PATTERN "*.py" PATTERN "*.ui"
  )

install(
    DIRECTORY expand
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING PATTERN "*.expand"
  )

install(
    DIRECTORY js_utils
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING PATTERN "*.py" PATTERN "*.ui" PATTERN "*.json"
    )

install(
    DIRECTORY python_console_ipython
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING PATTERN "*.py" PATTERN "*.ui" PATTERN "*.css"
  )

install(
    DIRECTORY cmake_utils
    DESTINATION ${DATA_INSTALL_DIR}/kate/pate
    FILES_MATCHING PATTERN "*.py" PATTERN "*.ui"
  )

# kate: indent-width 4;
