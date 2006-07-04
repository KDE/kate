
########### next target ###############

set(ktexteditor_isearch_PART_SRCS ISearchPlugin.cpp )

kde4_automoc(${ktexteditor_isearch_PART_SRCS})

kde4_add_plugin(ktexteditor_isearch ${ktexteditor_isearch_PART_SRCS})


target_link_libraries(ktexteditor_isearch  ${KDE4_KDECORE_LIBS} ktexteditor )

kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} ktexteditor_isearch )
install(TARGETS ktexteditor_isearch  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ktexteditor_isearch.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES ktexteditor_isearchui.rc  DESTINATION  ${DATA_INSTALL_DIR}/ktexteditor_isearch )




