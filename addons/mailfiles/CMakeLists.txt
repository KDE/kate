add_definitions(-DTRANSLATION_DOMAIN=\"katemailfilesplugin\")
set(katemailfilesplugin_PART_SRCS katemailfiles.cpp katemailfilesdialog.cpp )

kde4_add_plugin(katemailfilesplugin ${katemailfilesplugin_PART_SRCS})

target_link_libraries(katemailfilesplugin ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} kateinterfaces )

install(TARGETS katemailfilesplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

########### install files ###############

install( FILES ui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/kate/plugins/katemailfiles )
install( FILES katemailfilesplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
