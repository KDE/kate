cmake_minimum_required( VERSION 2.6 )

find_package( Qt4 REQUIRED )
find_package( KDE4 REQUIRED )
include( KDE4Defaults )
include_directories( ${QT_INCLUDES} )
include_directories( ${KDE4_INCLUDES} )
add_definitions( ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )

ADD_DEFINITIONS(-DKDE_DEFAULT_DEBUG_AREA=13040)

set( kategdbplugin_PART_SRCS plugin_kategdb.cpp debugview.cpp configview.cpp )
kde4_add_plugin( kategdbplugin ${kategdbplugin_PART_SRCS} )
target_link_libraries( kategdbplugin ${KDE4_KDEUI_LIBS} ${KDE4_KDECORE_LIBS} ${KDE4_KTEXTEDITOR_LIBS} kateinterfaces )

install( FILES ui.rc DESTINATION ${DATA_INSTALL_DIR}/kate/plugins/kategdb )
install( TARGETS kategdbplugin DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kategdbplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
kde4_install_icons( ${ICON_INSTALL_DIR} )
