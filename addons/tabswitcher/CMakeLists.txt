project(tabswitcherplugin)
add_definitions(-DTRANSLATION_DOMAIN=\"tabswitcherplugin\")

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(tabswitcherplugin_PART_SRCS
    tabswitcher.cpp
    tabswitchertreeview.cpp
)

# resource for ui file and stuff
qt5_add_resources(tabswitcherplugin_PART_SRCS plugin.qrc)

add_library (tabswitcherplugin MODULE ${tabswitcherplugin_PART_SRCS})

kcoreaddons_desktop_to_json (tabswitcherplugin tabswitcherplugin.desktop)

target_link_libraries(tabswitcherplugin
    KF5::TextEditor
    KF5::IconThemes
    KF5::I18n
    KF5::Service
)

install(TARGETS tabswitcherplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
