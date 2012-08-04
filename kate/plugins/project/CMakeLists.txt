# we need QJson
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
macro_optional_find_package(QJSON)
macro_log_feature(QJSON_FOUND "QJSON" "Json parser for Qt" "")

# bail out without it!
if (NOT QJSON_FOUND)
    message(WARNING "Project plugin needs QJSON Library.")
    return()
endif (NOT QJSON_FOUND)

set(kateprojectplugin_PART_SRCS plugin_kateproject.cpp )

kde4_add_plugin(kateprojectplugin ${kateprojectplugin_PART_SRCS})

target_link_libraries(kateprojectplugin  ${KDE4_KDEUI_LIBS} kateinterfaces )

########### install files ###############
install(TARGETS kateprojectplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/kateproject )
install( FILES kateproject.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

