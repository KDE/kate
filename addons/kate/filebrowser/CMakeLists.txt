project(katefilebrowserplugin)

# Load the frameworks we need
#find_package(KF5 CONFIG REQUIRED COMPONENTS ItemModels)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(katefilebrowserplugin_PART_SRCS
    katefilebrowserplugin.cpp
    katefilebrowserconfig.cpp
    katefilebrowser.cpp
    katebookmarkhandler.cpp
)

add_library(katefilebrowserplugin MODULE ${katefilebrowserplugin_PART_SRCS})

target_link_libraries(katefilebrowserplugin kateinterfaces KF5::KIOCore KF5::KIOWidgets)

install(TARGETS katefilebrowserplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

kservice_desktop_to_json (katefilebrowserplugin.desktop)
install( FILES katefilebrowserplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

