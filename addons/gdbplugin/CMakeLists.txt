# Hasn't been ported to Windows yet.
if(WIN32)
  return()
endif()

kcoreaddons_add_plugin(kategdbplugin INSTALL_NAMESPACE "ktexteditor")
target_compile_definitions(kategdbplugin PRIVATE TRANSLATION_DOMAIN="kategdbplugin")
target_link_libraries(kategdbplugin PRIVATE KF5::I18n KF5::TextEditor)

ki18n_wrap_ui(UI_SOURCES advanced_settings.ui)
target_sources(kategdbplugin PRIVATE ${UI_SOURCES})

target_sources(
  kategdbplugin
  PRIVATE
    plugin_kategdb.cpp
    debugview.cpp
    configview.cpp
    ioview.cpp
    localsview.cpp
    advanced_settings.cpp
    plugin.qrc 
)

