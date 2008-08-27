



########### next target ###############

set(katesymbolviewerplugin_PART_SRCS cpp_parser.cpp tcl_parser.cpp fortran_parser.cpp perl_parser.cpp 
php_parser.cpp xslt_parser.cpp ruby_parser.cpp python_parser.cpp plugin_katesymbolviewer.cpp )


kde4_add_plugin(katesymbolviewerplugin ${katesymbolviewerplugin_PART_SRCS})



target_link_libraries(katesymbolviewerplugin  ${KDE4_KDEUI_LIBS} kateinterfaces ${KDE4_KTEXTEDITOR_LIBS} )

install(TARGETS katesymbolviewerplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katesymbolviewer )
install( FILES katesymbolviewer.desktop
               katesymbolviewer_config.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
