project(tabswitcherplugin)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(tabswitcherplugin_PART_SRCS
    tabswitcher.cpp
    tabswitchertreeview.cpp
)

add_library (tabswitcherplugin MODULE ${tabswitcherplugin_PART_SRCS})

target_link_libraries(tabswitcherplugin
    KF5::TextEditor
    KF5::IconThemes
    KF5::I18n
    KF5::Service
)

install(TARGETS tabswitcherplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

########### install files ###############

install(FILES ui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/tabswitcher)

kcoreaddons_desktop_to_json (tabswitcherplugin tabswitcherplugin.desktop)
