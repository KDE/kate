



########### next target ###############

set(katehtmltoolsplugin_PART_SRCS plugin_katehtmltools.cpp )

kde4_automoc(${katehtmltoolsplugin_PART_SRCS})

kde4_add_plugin(katehtmltoolsplugin ${katehtmltoolsplugin_PART_SRCS})



target_link_libraries(katehtmltoolsplugin  ${KDE4_KDEUI_LIBS} kateinterfaces ktexteditor)

install(TARGETS katehtmltoolsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katehtmltools )
install( FILES katehtmltools.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
