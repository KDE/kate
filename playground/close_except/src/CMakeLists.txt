set(KATE_CLOSE_EXCEPT_PLUGIN_SOURCES
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
configure_file(katecloseexceptplugin.desktop.in katecloseexceptplugin.desktop)

install(TARGETS katecloseexceptplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/katecloseexceptplugin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/katecloseexceptplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})

# if (BUILD_TESTING)
#     add_subdirectory(test)
# endif()
