set(kateexternaltoolsplugin_PART_SRCS kateexternaltoolsplugin.cpp kateexternaltools.cpp )


set( kateexternaltoolsplugin_PART_UI
    configwidget.ui
)

kde4_add_ui_files(kateexternaltoolsplugin_PART_SRCS ${kateexternaltoolsplugin_PART_UI} )

kde4_add_plugin(kateexternaltoolsplugin ${kateexternaltoolsplugin_PART_SRCS})


target_link_libraries(kateexternaltoolsplugin ${KDE4_KDECORE_LIBS} ${KDE4_KTEXTEDITOR_LIBS} ${KDE4_KPARTS_LIBS} kateinterfaces )

install(TARGETS kateexternaltoolsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/kateexternaltools )
install( FILES kateexternaltoolsplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES externaltools DESTINATION ${DATA_INSTALL_DIR}/kate )
