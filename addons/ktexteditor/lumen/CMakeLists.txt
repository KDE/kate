project(ktexteditor_lumen)

########### next target ###############

set(ktexteditor_lumen_SRCS lumen.cpp dcd.cpp completion.cpp)

kde4_add_plugin(ktexteditor_lumen ${ktexteditor_lumen_SRCS})

target_link_libraries(ktexteditor_lumen ${KDE4_KDECORE_LIBS} ${KDE4_KTEXTEDITOR_LIBS})

install(TARGETS ktexteditor_lumen DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES ktexteditor_lumen.desktop DESTINATION ${SERVICES_INSTALL_DIR})