



########### next target ###############

set(katequickdocumentswitcherplugin_PART_SRCS plugin_katequickdocumentswitcher.cpp )


kde4_add_plugin(katequickdocumentswitcherplugin ${katequickdocumentswitcherplugin_PART_SRCS})



target_link_libraries(katequickdocumentswitcherplugin  ${KDE4_KDEUI_LIBS} ${KDE4_KTEXTEDITOR_LIBS} ${KDE4_KPARTS_LIBS} kateinterfaces )

install(TARGETS katequickdocumentswitcherplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katequickdocumentswitcher )
install( FILES katequickdocumentswitcher.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
