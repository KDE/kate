set(katemailfilesplugin_PART_SRCS katemailfiles.cpp katemailfilesdialog.cpp )

kde4_automoc(${katemailfilesplugin_PART_SRCS})

kde4_add_plugin(katemailfilesplugin ${katemailfilesplugin_PART_SRCS})


target_link_libraries(katemailfilesplugin ${KDE4_KDECORE_LIBS} kateinterfaces )

install(TARGETS katemailfilesplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katemailfiles )
install( FILES katemailfilesplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

