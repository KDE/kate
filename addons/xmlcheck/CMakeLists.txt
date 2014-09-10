########### next target ###############

add_definitions(-DTRANSLATION_DOMAIN=\"katexmlcheck\")

set(katexmlcheckplugin_PART_SRCS plugin_katexmlcheck.cpp )



kde4_add_plugin(katexmlcheckplugin ${katexmlcheckplugin_PART_SRCS})

target_link_libraries(katexmlcheckplugin ${KDE4_KDEUI_LIBS} ktexteditor kateinterfaces )

install(TARGETS katexmlcheckplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

########### install files ###############

install( FILES ui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/kate/plugins/katexmlcheck )
