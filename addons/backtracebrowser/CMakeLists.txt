kate_add_plugin(katebacktracebrowserplugin)
target_compile_definitions(katebacktracebrowserplugin PRIVATE TRANSLATION_DOMAIN="katebacktracebrowserplugin")
target_link_libraries(katebacktracebrowserplugin PRIVATE KF5::I18n KF5::TextEditor)

ki18n_wrap_ui(UI_SOURCES btbrowserwidget.ui btconfigwidget.ui)
target_sources(katebacktracebrowserplugin PRIVATE ${UI_SOURCES})

target_sources(
  katebacktracebrowserplugin
  PRIVATE
    katebacktracebrowser.cpp
    btparser.cpp
    btfileindexer.cpp
    btdatabase.cpp
)


if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
