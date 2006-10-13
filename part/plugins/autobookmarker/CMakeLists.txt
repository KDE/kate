
########### next target ###############

set(ktexteditor_autobookmarker_PART_SRCS autobookmarker.cpp )

kde4_automoc(ktexteditor_autobookmarker ${ktexteditor_autobookmarker_PART_SRCS})

kde4_add_plugin(ktexteditor_autobookmarker ${ktexteditor_autobookmarker_PART_SRCS})

target_link_libraries(ktexteditor_autobookmarker  ${KDE4_KDECORE_LIBS} ktexteditor )

install(TARGETS ktexteditor_autobookmarker  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ktexteditor_autobookmarkerrc  DESTINATION  ${CONFIG_INSTALL_DIR} )
install( FILES ktexteditor_autobookmarker.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




