



########### next target ###############

set(katepybrowseplugin_PART_SRCS kpybrowser.cpp pybrowsenode.cpp pybrowse_part.cpp )

kde4_automoc(${katepybrowseplugin_PART_SRCS})

kde4_add_plugin(katepybrowseplugin ${katepybrowseplugin_PART_SRCS})



target_link_libraries(katepybrowseplugin  ${KDE4_KDEUI_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces )

install(TARGETS katepybrowseplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katepybrowse )
install( FILES katepybrowse.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

