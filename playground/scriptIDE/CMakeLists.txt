
# building separately or as part of kdesdk?
if(NOT KDE4_FOUND)
    cmake_minimum_required( VERSION 2.6 )
    find_package(KDE4 REQUIRED)
    ADD_DEFINITIONS(-DKDE_DEFAULT_DEBUG_AREA=13040)
    include_directories(
      ${KDE4_INCLUDES}
      ${KDE4_KIO_INCLUDES}
      ${CMAKE_CURRENT_BINARY_DIR}
      ${QT_INCLUDES}
      ../../kate/interfaces
    )
else(NOT KDE4_FOUND)
    include_directories(
      ../../kate
      ../../kate/interfaces
      ../../kate/app
     )
endif(NOT KDE4_FOUND)


set(scriptIDEplugin_PART_SRCS
    scriptIDEplugin.cpp
)

kde4_add_ui_files(scriptIDEplugin_PART_SRCS scriptIDE.ui)

kde4_add_plugin(scriptIDEplugin ${scriptIDEplugin_PART_SRCS})

target_link_libraries(scriptIDEplugin ${KDE4_KDEUI_LIBS} ${KDE4_KDECORE_LIBS} ${KDE4_KTEXTEDITOR_LIBS} ${QT_QTSCRIPT_LIBRARY} kateinterfaces)

########### install files ###############
install(TARGETS scriptIDEplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/scriptIDE)
install(FILES scriptIDE.desktop DESTINATION ${SERVICES_INSTALL_DIR})

