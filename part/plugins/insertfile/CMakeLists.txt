
########### next target ###############

set(ktexteditor_insertfile_PART_SRCS insertfileplugin.cpp )

kde4_automoc(${ktexteditor_insertfile_PART_SRCS})

kde4_add_plugin(ktexteditor_insertfile ${ktexteditor_insertfile_PART_SRCS})

target_link_libraries(ktexteditor_insertfile  ${KDE4_KDECORE_LIBS} ktexteditor )

kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} ktexteditor_insertfile )
install_targets(${PLUGIN_INSTALL_DIR}  ktexteditor_insertfile )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES ktexteditor_insertfile.desktop )
install_files( ${DATA_INSTALL_DIR}/ktexteditor_insertfile FILES ktexteditor_insertfileui.rc )




