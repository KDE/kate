project(ktexteditor_autobrace)

add_subdirectory( icons )

########### next target ###############

set(ktexteditor_autobrace_PART_SRCS autobrace.cpp autobrace_config.cpp)

kde4_add_plugin(ktexteditor_autobrace ${ktexteditor_autobrace_PART_SRCS} )

target_link_libraries(ktexteditor_autobrace ${KDE4_KDECORE_LIBS} ${KDE4_KTEXTEDITOR_LIBS} )

install(TARGETS ktexteditor_autobrace DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install(FILES ktexteditor_autobrace.desktop ktexteditor_autobrace_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
