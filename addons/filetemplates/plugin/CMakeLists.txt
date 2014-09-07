########### next target ###############

set(katefiletemplates_PART_SRCS filetemplates.cpp )

kde4_add_plugin(katefiletemplates ${katefiletemplates_PART_SRCS})

target_link_libraries(katefiletemplates kateinterfaces ktexteditor )

install(TARGETS katefiletemplates  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/kate/plugins/katefiletemplates )
install( FILES katetemplate.xml  DESTINATION  ${DATA_INSTALL_DIR}/katepart5/syntax )
install( FILES katefiletemplates.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
