
########### next target ###############

set(ktexteditor_docwordcompletion_PART_SRCS docwordcompletion.cpp )

kde4_add_plugin(ktexteditor_docwordcompletion ${ktexteditor_docwordcompletion_PART_SRCS} )

target_link_libraries(ktexteditor_docwordcompletion ${KDE4_KDECORE_LIBS} ktexteditor )

install(TARGETS ktexteditor_docwordcompletion DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

set(kcm_ktexteditor_docwordcompletion_PART_SRCS docwordcompletion_config.cpp )

kde4_add_plugin(kcm_ktexteditor_docwordcompletion ${kcm_ktexteditor_docwordcompletion_PART_SRCS} )

target_link_libraries(kcm_ktexteditor_docwordcompletion ${KDE4_KDECORE_LIBS} ktexteditor )

install(TARGETS kcm_ktexteditor_docwordcompletion DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ktexteditor_docwordcompletion.desktop
               ktexteditor_docwordcompletion_config.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )




