


########### next target ###############

set(katefilebrowserplugin_PART_SRCS katefileselector.cpp kbookmarkhandler.cpp )

kde4_automoc(${katefilebrowserplugin_PART_SRCS})

kde4_add_plugin(katefilebrowserplugin ${katefilebrowserplugin_PART_SRCS})


target_link_libraries(katefilebrowserplugin ${KDE4_KFILE_LIBS} kateinterfaces )

install(TARGETS katefilebrowserplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES katefilebrowserplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

