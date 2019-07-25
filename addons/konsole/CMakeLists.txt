add_library(katekonsoleplugin MODULE "")
target_compile_definitions(katekonsoleplugin PRIVATE TRANSLATION_DOMAIN="katekonsoleplugin")

target_link_libraries(
  katekonsoleplugin 
  PRIVATE
    KF5::TextEditor
    KF5::Parts
    KF5::IconThemes
    KF5::I18n
    KF5::Service
)

target_sources(
  katekonsoleplugin 
  PRIVATE
    kateconsole.cpp 
    plugin.qrc
)

kcoreaddons_desktop_to_json(katekonsoleplugin katekonsoleplugin.desktop)
install(TARGETS katekonsoleplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
