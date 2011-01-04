cmake_minimum_required( VERSION 2.6 )

# building separately or as part of kdesdk?
if(NOT KDE4_FOUND)
    find_package(KDE4 REQUIRED)
    ADD_DEFINITIONS(-DKDE_DEFAULT_DEBUG_AREA=13040)
endif(NOT KDE4_FOUND)

include_directories(${KDE4_INCLUDES})
include_directories(${KDE4_KIO_INCLUDES})
include_directories(${CMAKE_CURRENT_BINARY_DIR})


set(kategdbplugin_PART_SRCS 
    plugin_kategdb.cpp 
    debugview.cpp 
    configview.cpp 
    ioview.cpp
    localsview.cpp
)

kde4_add_plugin( kategdbplugin ${kategdbplugin_PART_SRCS} )
target_link_libraries( kategdbplugin ${KDE4_KDEUI_LIBS} ${KDE4_KDECORE_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces )

install( FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/kategdb )
install( TARGETS kategdbplugin DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kategdbplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
kde4_install_icons( ${ICON_INSTALL_DIR} )
