

########### next target ###############

set(katexmlcheckplugin_PART_SRCS plugin_katexmlcheck.cpp )

kde4_add_plugin(katexmlcheckplugin ${katexmlcheckplugin_PART_SRCS})

target_link_libraries(katexmlcheckplugin ${KDE4_KDEUI_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces )

install(TARGETS katexmlcheckplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katexmlcheck )
install( FILES katexmlcheck.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )



