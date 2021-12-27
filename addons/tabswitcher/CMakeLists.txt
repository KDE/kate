kate_add_plugin(tabswitcherplugin)
target_compile_definitions(tabswitcherplugin PRIVATE TRANSLATION_DOMAIN="tabswitcherplugin")
target_link_libraries(tabswitcherplugin PRIVATE KF5::I18n KF5::TextEditor)

target_sources(
  tabswitcherplugin 
  PRIVATE
    tabswitcher.cpp
    tabswitcherfilesmodel.cpp
    tabswitchertreeview.cpp
    plugin.qrc
)


if(BUILD_TESTING)
  add_subdirectory(autotests)
  add_subdirectory(tests)
endif()
