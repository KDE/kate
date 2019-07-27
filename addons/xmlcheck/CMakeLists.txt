remove_definitions(-DQT_NO_CAST_TO_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

add_library(katexmlcheckplugin MODULE ${katexmlcheckplugin_PART_SRCS})
target_compile_definitions(katexmlcheckplugin PRIVATE TRANSLATION_DOMAIN="katexmlcheck")
target_link_libraries(katexmlcheckplugin PRIVATE KF5::TextEditor)

target_sources(
  katexmlcheckplugin 
  PRIVATE
    plugin_katexmlcheck.cpp
    plugin.qrc
)

kcoreaddons_desktop_to_json(katexmlcheckplugin katexmlcheck.desktop)
install(TARGETS katexmlcheckplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
