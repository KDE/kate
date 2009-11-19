project(ktexteditor_autobrace)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

add_subdirectory( icons )

########### next target ###############

include_directories(${KDE4_INCLUDES} ${QT_INCLUDES})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

set(ktexteditor_autobrace_PART_SRCS autobrace.cpp )

kde4_add_plugin(ktexteditor_autobrace ${ktexteditor_autobrace_PART_SRCS} )

target_link_libraries(ktexteditor_autobrace ${KDE4_KDECORE_LIBS} ${KDE4_KTEXTEDITOR_LIBS} )

install(TARGETS ktexteditor_autobrace DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install(FILES ktexteditor_autobrace.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
