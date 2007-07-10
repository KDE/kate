set(kateexternaltoolsplugin_PART_SRCS kateexternaltoolsplugin.cpp kateexternaltools.cpp )


kde4_add_plugin(kateexternaltoolsplugin ${kateexternaltoolsplugin_PART_SRCS})


target_link_libraries(kateexternaltoolsplugin ${KDE4_KDECORE_LIBS} kateinterfaces )

install(TARGETS kateexternaltoolsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/kateexternaltools )
install( FILES kateexternaltoolsplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

