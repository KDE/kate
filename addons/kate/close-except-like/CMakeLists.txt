set(VERSION_MAJOR 0)
set(VERSION_MINOR 3)
set(VERSION_PATCH 5)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

set(KATE_CLOSE_EXCEPT_PLUGIN_SOURCES
    close_confirm_dialog.cpp
    close_except_plugin.cpp
  )

kde4_add_plugin(katecloseexceptplugin ${KATE_CLOSE_EXCEPT_PLUGIN_SOURCES})

target_link_libraries(katecloseexceptplugin
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KFILE_LIBS}
    ${KDE4_KTEXTEDITOR_LIBS}
    kateinterfaces
  )

configure_file(config.h.in config.h)

install(TARGETS katecloseexceptplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/katecloseexceptplugin)
install(FILES katecloseexceptplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
