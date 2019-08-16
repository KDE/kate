add_library(katebacktracebrowserplugin MODULE "")
target_compile_definitions(katebacktracebrowserplugin PRIVATE TRANSLATION_DOMAIN="katebacktracebrowserplugin")
target_link_libraries(katebacktracebrowserplugin PRIVATE KF5::TextEditor)

ki18n_wrap_ui(UI_SRCS btbrowserwidget.ui btconfigwidget.ui)
target_sources(katebacktracebrowserplugin PRIVATE ${UI_SRCS})

target_sources(
  katebacktracebrowserplugin
  PRIVATE
    katebacktracebrowser.cpp
    btparser.cpp
    btfileindexer.cpp
    btdatabase.cpp
)

kcoreaddons_desktop_to_json(katebacktracebrowserplugin katebacktracebrowserplugin.desktop)
install(TARGETS katebacktracebrowserplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
