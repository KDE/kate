project(ArtisticComment)

find_package(KDE4 REQUIRED)
include (KDE4Defaults)
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -fpermissive)
include_directories(${KDE4_INCLUDES})

set(acomment_SRCS artisticcomment.cpp)
set(ktexteditor_acomment_SRCS acommentplugin.cpp)
kde4_add_ui_files(ktexteditor_acomment_SRCS styledialog.ui)

add_library(acomment STATIC ${acomment_SRCS})
target_link_libraries(acomment QtCore)

kde4_add_plugin(ktexteditor_acomment ${ktexteditor_acomment_SRCS} ${acomment_SRCS})
target_link_libraries(ktexteditor_acomment ${KDE4_KDEUI_LIBS} ktexteditor)

install(TARGETS acomment ${INSTALL_TARGETS_DEFAULT_ARGS})

install(TARGETS ktexteditor_acomment DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES acommentui.rc DESTINATION ${DATA_INSTALL_DIR}/ktexteditor_acomment)

install(FILES ktexteditor_acomment.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES artisticcommentrc DESTINATION ${CONFIG_INSTALL_DIR})

install(FILES artisticcomment.h DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)
