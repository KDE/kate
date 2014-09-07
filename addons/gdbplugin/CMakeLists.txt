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

add_library(kategdbplugin MODULE ${kategdbplugin_PART_SRCS})

target_link_libraries(kategdbplugin KF5::TextEditor KF5::I18n KF5::IconThemes)

########### install files ###############
install(FILES ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/kategdb)
install(TARGETS kategdbplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
install(FILES kategdbplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
ecm_install_icons (${ICON_INSTALL_DIR})
kcoreaddons_desktop_to_json (kategdbplugin kategdbplugin.desktop)
