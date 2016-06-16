project(katexmltoolsplugin)
add_definitions(-DTRANSLATION_DOMAIN=\"katexmltools\")

# Maybe remove these later
remove_definitions(-DQT_NO_CAST_TO_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

set(katexmltoolsplugin_PART_SRCS
    pseudo_dtd.cpp
    plugin_katexmltools.cpp
)

# resource for ui file and stuff
qt5_add_resources(katexmltoolsplugin_PART_SRCS plugin.qrc)

add_library(katexmltoolsplugin MODULE ${katexmltoolsplugin_PART_SRCS})

target_link_libraries(katexmltoolsplugin
    KF5::TextEditor
    KF5::Parts
    KF5::IconThemes
    KF5::I18n
    KF5::Service
)

########### install files ###############
install(TARGETS katexmltoolsplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
install( FILES html4-loose.dtd.xml  html4-strict.dtd.xml kde-docbook.dtd.xml simplify_dtd.xsl
               xhtml1-frameset.dtd.xml xhtml1-strict.dtd.xml xhtml1-transitional.dtd.xml xslt-1.0.dtd.xml
               testcases.xml language.dtd.xml kpartgui.dtd.xml kcfg.dtd.xml
               DESTINATION  ${DATA_INSTALL_DIR}/katexmltools )

kcoreaddons_desktop_to_json (katexmltoolsplugin katexmltools.desktop)
