
########### next target ###############

set(ktexteditor_insertfile_PART_SRCS insertfileplugin.cpp )


kde4_add_plugin(ktexteditor_insertfile ${ktexteditor_insertfile_PART_SRCS})

target_link_libraries(ktexteditor_insertfile ${KDE4_KIO_LIBS} ${KDE4_KTEXTEDITOR_LIBS})

install(TARGETS  ktexteditor_insertfile  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ktexteditor_insertfile.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES ktexteditor_insertfileui.rc  DESTINATION  ${DATA_INSTALL_DIR}/ktexteditor_insertfile )




