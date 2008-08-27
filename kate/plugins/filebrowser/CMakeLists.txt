
add_subdirectory( icons )


########### next target ###############

set(katefilebrowserplugin_PART_SRCS katefileselector.cpp kbookmarkhandler.cpp )


kde4_add_plugin(katefilebrowserplugin ${katefilebrowserplugin_PART_SRCS})


target_link_libraries(katefilebrowserplugin ${KDE4_KFILE_LIBS} ${KDE4_KPARTS_LIBS} kateinterfaces )

install(TARGETS katefilebrowserplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES katefilebrowserplugin.desktop
               katefilebrowserplugin_config.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
