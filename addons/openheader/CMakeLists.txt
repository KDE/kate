add_library(kateopenheaderplugin MODULE "")
target_compile_definitions(kateopenheaderplugin PRIVATE TRANSLATION_DOMAIN="kateopenheader")
target_link_libraries(kateopenheaderplugin PRIVATE KF5::I18n KF5::TextEditor)

target_sources(
  kateopenheaderplugin 
  PRIVATE
    plugin_kateopenheader.cpp 
    plugin.qrc
)

kcoreaddons_desktop_to_json(kateopenheaderplugin kateopenheaderplugin.desktop)

# ensure we are able to load plugins pre-install, too, directories must match!
set_target_properties(kateopenheaderplugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/ktexteditor")
install(TARGETS kateopenheaderplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
