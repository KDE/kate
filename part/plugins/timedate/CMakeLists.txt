
########### next target ###############

set(ktexteditor_timedate_PART_SRCS timedate.cpp timedate_config.cpp )

kde4_add_plugin(ktexteditor_timedate ${ktexteditor_timedate_PART_SRCS} )

target_link_libraries(ktexteditor_timedate ${KDE4_KDECORE_LIBS} ktexteditor )

install(TARGETS ktexteditor_timedate DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install(FILES timedateui.rc DESTINATION ${DATA_INSTALL_DIR}/ktexteditor_timedate )

install(FILES ktexteditor_timedate.desktop
              ktexteditor_timedate_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
