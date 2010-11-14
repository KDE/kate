



########### next target ###############

set(kateinsertcommandplugin_PART_SRCS plugin_kateinsertcommand.cpp )


kde4_add_plugin(kateinsertcommandplugin ${kateinsertcommandplugin_PART_SRCS})



target_link_libraries(kateinsertcommandplugin  ${KDE4_KDECORE_LIBS} kateinterfaces )

install(TARGETS kateinsertcommandplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/kateinsertcommand )
install( FILES kateinsertcommand.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

