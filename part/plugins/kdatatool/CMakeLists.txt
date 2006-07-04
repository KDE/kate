
########### next target ###############

set(ktexteditor_kdatatool_PART_SRCS kate_kdatatool.cpp )

kde4_automoc(${ktexteditor_kdatatool_PART_SRCS})

kde4_add_plugin(ktexteditor_kdatatool ${ktexteditor_kdatatool_PART_SRCS})

target_link_libraries(ktexteditor_kdatatool  ${KDE4_KDECORE_LIBS} ktexteditor )

kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} ktexteditor_kdatatool )
install(TARGETS ktexteditor_kdatatool  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ktexteditor_kdatatoolui.rc  DESTINATION  ${DATA_INSTALL_DIR}/ktexteditor_kdatatool )
install( FILES ktexteditor_kdatatool.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




