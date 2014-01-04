
########### install files ###############


# install the export file
install( FILES ktexteditor.desktop ktexteditorplugin.desktop ktexteditorapplicationplugin.desktop DESTINATION  ${SERVICETYPES_INSTALL_DIR} )

########### export targets ##############

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/KTextEditor")

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KTextEditorConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KTextEditorConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KTextEditor DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KTextEditorTargets.cmake NAMESPACE KTextEditor:: )

install( FILES katepartui.rc DESTINATION  ${DATA_INSTALL_DIR}/katepart )

install( FILES katemoderc kateschemarc katesyntaxhighlightingrc DESTINATION  ${CONFIG_INSTALL_DIR} )

kservice_desktop_to_json (katepart.desktop)
install( FILES katepart.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
