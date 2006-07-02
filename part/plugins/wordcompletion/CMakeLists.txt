
########### next target ###############

set(ktexteditor_docwordcompletion_PART_SRCS docwordcompletion.cpp )

kde4_automoc(${ktexteditor_docwordcompletion_PART_SRCS})

kde4_add_plugin(ktexteditor_docwordcompletion ${ktexteditor_docwordcompletion_PART_SRCS})

target_link_libraries(ktexteditor_docwordcompletion  ${KDE4_KDECORE_LIBS} ktexteditor )

kde4_install_libtool_file(${PLUGIN_INSTALL_DIR} ktexteditor_docwordcompletion )
install_targets(${PLUGIN_INSTALL_DIR} ktexteditor_docwordcompletion )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/ktexteditor_docwordcompletion FILES docwordcompletionui.rc )
install_files( ${SERVICES_INSTALL_DIR} FILES ktexteditor_docwordcompletion.desktop )




