
########### next target ###############

set(ktexteditor_docwordcompletion_PART_SRCS docwordcompletion.cpp )

kde4_automoc(${ktexteditor_docwordcompletion_PART_SRCS})

kde4_add_plugin(ktexteditor_docwordcompletion ${ktexteditor_docwordcompletion_PART_SRCS})

target_link_libraries(ktexteditor_docwordcompletion  ${KDE4_KDECORE_LIBS} ktexteditor )

install(TARGETS ktexteditor_docwordcompletion  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES docwordcompletionui.rc  DESTINATION  ${DATA_INSTALL_DIR}/ktexteditor_docwordcompletion )
install( FILES ktexteditor_docwordcompletion.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




