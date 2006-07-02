
########### next target ###############

set(ktexteditor_kdatatool_PART_SRCS kate_kdatatool.cpp )

kde4_automoc(${ktexteditor_kdatatool_PART_SRCS})

kde4_add_plugin(ktexteditor_kdatatool ${ktexteditor_kdatatool_PART_SRCS})

target_link_libraries(ktexteditor_kdatatool  ${KDE4_KDECORE_LIBS} ktexteditor )

kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} ktexteditor_kdatatool )
install_targets(${PLUGIN_INSTALL_DIR} ktexteditor_kdatatool )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/ktexteditor_kdatatool FILES ktexteditor_kdatatoolui.rc )
install_files( ${SERVICES_INSTALL_DIR} FILES ktexteditor_kdatatool.desktop )




