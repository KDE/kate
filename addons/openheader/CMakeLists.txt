add_library(kateopenheaderplugin MODULE "")
target_compile_definitions(kateopenheaderplugin PRIVATE TRANSLATION_DOMAIN="kateopenheader")

target_link_libraries(
  kateopenheaderplugin 
  PRIVATE
    KF5::TextEditor
    KF5::I18n
    KF5::Parts
)

target_sources(
  kateopenheaderplugin 
  PRIVATE
    plugin_kateopenheader.cpp 
    plugin.qrc
)

kcoreaddons_desktop_to_json(kateopenheaderplugin kateopenheaderplugin.desktop)
install(TARGETS kateopenheaderplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
