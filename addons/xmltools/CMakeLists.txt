remove_definitions(-DQT_NO_CAST_TO_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

add_library(katexmltoolsplugin MODULE "")
target_compile_definitions(katexmltoolsplugin PRIVATE TRANSLATION_DOMAIN="katexmltools")
target_link_libraries(katexmltoolsplugin PRIVATE KF5::TextEditor)

target_sources(
  katexmltoolsplugin 
  PRIVATE
    pseudo_dtd.cpp
    plugin_katexmltools.cpp
    plugin.qrc
)

kcoreaddons_desktop_to_json(katexmltoolsplugin katexmltools.desktop)
install(TARGETS katexmltoolsplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

install(
  FILES 
    html4-loose.dtd.xml 
    html4-strict.dtd.xml 
    kde-docbook.dtd.xml 
    simplify_dtd.xsl
    xhtml1-frameset.dtd.xml 
    xhtml1-strict.dtd.xml 
    xhtml1-transitional.dtd.xml 
    xslt-1.0.dtd.xml
    testcases.xml 
    language.dtd.xml 
    kpartgui.dtd.xml 
    kcfg.dtd.xml
  DESTINATION ${DATA_INSTALL_DIR}/katexmltools
)
