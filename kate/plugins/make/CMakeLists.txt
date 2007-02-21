



########### next target ###############

set(katemakeplugin_PART_SRCS plugin_katemake.cpp )

kde4_automoc(${katemakeplugin_PART_SRCS})

kde4_add_plugin(katemakeplugin ${katemakeplugin_PART_SRCS})



target_link_libraries(katemakeplugin  ${KDE4_KDECORE_LIBS} kateinterfaces )

install(TARGETS katemakeplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katemake )
install( FILES katemake.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

