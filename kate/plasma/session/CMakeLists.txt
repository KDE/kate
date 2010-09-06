project(katesessionapplet)

set(katesessionapplet_SRCS
    katesessionapplet.cpp)

kde4_add_ui_files(katesessionapplet_SRCS
                  katesessionConfig.ui)

kde4_add_plugin(plasma_applet_katesession ${katesessionapplet_SRCS})
target_link_libraries(plasma_applet_katesession ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS plasma_applet_katesession DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-applet-katesession.desktop DESTINATION ${SERVICES_INSTALL_DIR})
