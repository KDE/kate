add_library (tabswitcherplugin MODULE "")
target_compile_definitions(tabswitcherplugin PRIVATE TRANSLATION_DOMAIN="tabswitcherplugin")
target_link_libraries(tabswitcherplugin PRIVATE KF5::TextEditor)

target_sources(
  tabswitcherplugin 
  PRIVATE
    tabswitcher.cpp
    tabswitcherfilesmodel.cpp
    tabswitchertreeview.cpp
    plugin.qrc
)

kcoreaddons_desktop_to_json(tabswitcherplugin tabswitcherplugin.desktop)
install(TARGETS tabswitcherplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

if(BUILD_TESTING)
  add_subdirectory(autotests)
  add_subdirectory(tests)
endif()
