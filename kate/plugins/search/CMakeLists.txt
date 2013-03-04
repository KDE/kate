
# building separately or as part of kdesdk?
if(NOT KDE4_FOUND)
    cmake_minimum_required( VERSION 2.6 )
    find_package(KDE4 REQUIRED)
    ADD_DEFINITIONS(-DKDE_DEFAULT_DEBUG_AREA=13040)
    include_directories(${KDE4_INCLUDES})
    include_directories(${KDE4_KIO_INCLUDES})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif(NOT KDE4_FOUND)


set(katesearchplugin_PART_SRCS
    plugin_search.cpp
    search_open_files.cpp
    SearchDiskFiles.cpp
    FolderFilesList.cpp
    replace_matches.cpp
    htmldelegate.cpp
)

kde4_add_ui_files(katesearchplugin_PART_SRCS search.ui results.ui)

kde4_add_plugin(katesearchplugin ${katesearchplugin_PART_SRCS})

target_link_libraries(katesearchplugin ${KDE4_KDEUI_LIBS} ${KDE4_KDECORE_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces)

########### install files ###############
install(TARGETS katesearchplugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/katesearch)
install(FILES katesearch.desktop DESTINATION ${SERVICES_INSTALL_DIR})

