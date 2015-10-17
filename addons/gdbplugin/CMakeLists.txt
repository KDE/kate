add_definitions(-DTRANSLATION_DOMAIN=\"kategdbplugin\")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(kategdbplugin_PART_SRCS
    plugin_kategdb.cpp
    debugview.cpp
    configview.cpp
    ioview.cpp
    localsview.cpp
    advanced_settings.cpp
)

ki18n_wrap_ui(kategdbplugin_PART_SRCS advanced_settings.ui)

# resource for ui file and stuff
qt5_add_resources(kategdbplugin_PART_SRCS plugin.qrc)

add_library(kategdbplugin MODULE ${kategdbplugin_PART_SRCS})
kcoreaddons_desktop_to_json (kategdbplugin kategdbplugin.desktop)

target_link_libraries(kategdbplugin KF5::TextEditor KF5::I18n KF5::IconThemes)

install(TARGETS kategdbplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
