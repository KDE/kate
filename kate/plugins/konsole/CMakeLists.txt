

########### next target ###############

set(katekonsoleplugin_PART_SRCS kateconsole.cpp )


kde4_add_plugin(katekonsoleplugin ${katekonsoleplugin_PART_SRCS})


target_link_libraries(katekonsoleplugin  ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} kateinterfaces )

install(TARGETS katekonsoleplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katekonsole )
install( FILES katekonsoleplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

