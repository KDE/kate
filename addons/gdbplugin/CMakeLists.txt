# Hasn't been ported to Windows yet.
if(WIN32)
  return()
endif()

set(kategdbplugin_PART_SRCS
  plugin_kategdb.cpp
  debugview.cpp
  configview.cpp
  ioview.cpp
  localsview.cpp
  advanced_settings.cpp
  plugin.qrc
)

ki18n_wrap_ui(kategdbplugin_PART_SRCS advanced_settings.ui)

add_library(kategdbplugin MODULE ${kategdbplugin_PART_SRCS})
target_compile_definitions(kategdbplugin PRIVATE TRANSLATION_DOMAIN="kategdbplugin")
target_link_libraries(kategdbplugin PRIVATE KF5::TextEditor)

kcoreaddons_desktop_to_json(kategdbplugin kategdbplugin.desktop)
install(TARGETS kategdbplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
