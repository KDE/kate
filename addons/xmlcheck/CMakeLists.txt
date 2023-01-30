remove_definitions(-DQT_NO_CAST_TO_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

kate_add_plugin(katexmlcheckplugin)
target_compile_definitions(katexmlcheckplugin PRIVATE TRANSLATION_DOMAIN="katexmlcheck")
target_link_libraries(katexmlcheckplugin PRIVATE kateprivate KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor)

target_sources(
  katexmlcheckplugin
  PRIVATE
    plugin_katexmlcheck.cpp
    plugin.qrc
)
if (QT_MAJOR_VERSION STREQUAL "6")
	target_link_libraries( katexmlcheckplugin PRIVATE Qt::Core5Compat)
endif()
