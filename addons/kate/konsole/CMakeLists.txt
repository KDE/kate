project(katekonsoleplugin)

# Load the frameworks we need
find_package(KF5 CONFIG REQUIRED COMPONENTS ItemModels)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(katekonsoleplugin_PART_SRCS kateconsole.cpp )

add_library (katekonsoleplugin MODULE ${katekonsoleplugin_PART_SRCS})

target_link_libraries(katekonsoleplugin
    kateinterfaces
    KF5::KDE4Support
    KF5::Parts
    KF5::NewStuff Qt5::Script KF5::SonnetCore KF5::Archive KF5::KCMUtils KF5::PrintUtils KF5::ItemViews KF5::ItemModels
)

install(TARGETS katekonsoleplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katekonsole )

kservice_desktop_to_json (katekonsoleplugin.desktop)
install( FILES katekonsoleplugin.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )

