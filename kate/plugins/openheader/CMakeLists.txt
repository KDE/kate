



########### next target ###############

set(kateopenheaderplugin_PART_SRCS plugin_kateopenheader.cpp )


kde4_add_plugin(kateopenheaderplugin ${kateopenheaderplugin_PART_SRCS})



target_link_libraries(kateopenheaderplugin  kateinterfaces ${KDE4_KIO_LIBS})

install(TARGETS kateopenheaderplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/kateopenheader )
install( FILES kateopenheader.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
