
########### next target ###############

set(ktexteditor_isearch_PART_SRCS ISearchPlugin.cpp )

kde4_automoc(${ktexteditor_isearch_PART_SRCS})

kde4_add_plugin(ktexteditor_isearch ${ktexteditor_isearch_PART_SRCS})


target_link_libraries(ktexteditor_isearch  ${KDE4_KDECORE_LIBS} ktexteditor )

kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} ktexteditor_isearch )
install_targets(${PLUGIN_INSTALL_DIR} ktexteditor_isearch )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES ktexteditor_isearch.desktop )
install_files( ${DATA_INSTALL_DIR}/ktexteditor_isearch FILES ktexteditor_isearchui.rc )




