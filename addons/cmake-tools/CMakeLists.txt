add_library(cmaketoolsplugin MODULE "")
target_compile_definitions(cmaketoolsplugin PRIVATE TRANSLATION_DOMAIN="cmaketoolsplugin")
target_link_libraries(cmaketoolsplugin PRIVATE kateprivate KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor)

target_sources(
  cmaketoolsplugin
  PRIVATE
    cmaketoolsplugin.cpp
    cmakecompletion.cpp
)

# ensure we are able to load plugins pre-install, too, directories must match!
set_target_properties(cmaketoolsplugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/ktexteditor")
install(TARGETS cmaketoolsplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
