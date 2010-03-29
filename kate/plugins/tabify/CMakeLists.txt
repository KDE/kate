find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include_directories(${KDE4_INCLUDES})

kde4_add_plugin(katetabifyplugin tabify.cpp)

target_link_libraries(katetabifyplugin  ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} kateinterfaces)

install(TARGETS katetabifyplugin DESTINATION ${PLUGIN_INSTALL_DIR})

####################### install files #########################################
install(FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/katetabify)
install(FILES  katetabifyplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
