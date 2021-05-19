remove_definitions(-DQT_NO_CAST_TO_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

kcoreaddons_add_plugin(katexmlcheckplugin INSTALL_NAMESPACE "ktexteditor")
target_compile_definitions(katexmlcheckplugin PRIVATE TRANSLATION_DOMAIN="katexmlcheck")
target_link_libraries(katexmlcheckplugin PRIVATE KF5::I18n KF5::TextEditor)

target_sources(
  katexmlcheckplugin 
  PRIVATE
    plugin_katexmlcheck.cpp
    plugin.qrc
)

