project(katefilebrowserplugin)

# Load the frameworks we need
find_package(KF5 CONFIG REQUIRED COMPONENTS ItemModels)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(katefilebrowserplugin_PART_SRCS
    katefilebrowserplugin.cpp
    katefilebrowserconfig.cpp
    katefilebrowser.cpp
    katebookmarkhandler.cpp
)

add_library(katefilebrowserplugin MODULE ${katefilebrowserplugin_PART_SRCS})

target_link_libraries(katefilebrowserplugin
    kateinterfaces
    KF5::Parts
    KF5::KDE4Support
    KF5::NewStuff Qt5::Script KF5::SonnetCore KF5::Archive KF5::KCMUtils KF5::PrintUtils KF5::ItemViews KF5::ItemModels
)

install(TARGETS katefilebrowserplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

kservice_desktop_to_json (katefilebrowserplugin.desktop)
install( FILES katefilebrowserplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

