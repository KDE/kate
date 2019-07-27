set(katebacktracebrowserplugin_PART_SRCS
  katebacktracebrowser.cpp
  btparser.cpp
  btfileindexer.cpp
  btdatabase.cpp
)

set(katebacktracebrowserplugin_PART_UI
  btbrowserwidget.ui
  btconfigwidget.ui
)

ki18n_wrap_ui(katebacktracebrowserplugin_PART_SRCS ${katebacktracebrowserplugin_PART_UI})

add_library(katebacktracebrowserplugin MODULE ${katebacktracebrowserplugin_PART_SRCS})
target_compile_definitions(katebacktracebrowserplugin PRIVATE TRANSLATION_DOMAIN="katebacktracebrowserplugin")
target_link_libraries(katebacktracebrowserplugin PRIVATE KF5::TextEditor)

kcoreaddons_desktop_to_json(katebacktracebrowserplugin katebacktracebrowserplugin.desktop)
install(TARGETS katebacktracebrowserplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
