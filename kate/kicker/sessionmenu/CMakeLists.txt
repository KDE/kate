########### next target ###############

find_library(KICKER_WORKSPACE_LIBRARY NAMES kworkspace PATHS ${KDE4_LIB_INSTALL_DIR} NO_DEFAULT_PATH )

set(kickermenu_kate_PART_SRCS katesessionmenu.cpp )


kde4_add_plugin(kickermenu_kate ${kickermenu_kate_PART_SRCS})


target_link_libraries(kickermenu_kate ${KICKER_WORKSPACE_LIBRARY} kickermain ${KDE4_KDEUI_LIBS})

install(TARGETS kickermenu_kate DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES katesessionmenu.desktop DESTINATION  ${DATA_INSTALL_DIR}/kicker/menuext )
