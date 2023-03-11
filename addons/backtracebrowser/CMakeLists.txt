kate_add_plugin(katebacktracebrowserplugin)
target_compile_definitions(katebacktracebrowserplugin PRIVATE TRANSLATION_DOMAIN="katebacktracebrowserplugin")
target_link_libraries(katebacktracebrowserplugin PRIVATE KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor KF${KF_MAJOR_VERSION}::KIOWidgets)

ki18n_wrap_ui(katebacktracebrowserplugin btbrowserwidget.ui btconfigwidget.ui)
target_sources(
  katebacktracebrowserplugin
  PRIVATE
    katebacktracebrowser.cpp
    btparser.cpp
    btfileindexer.cpp
    btdatabase.cpp
)

if (BUILD_PCH)
    target_precompile_headers(katebacktracebrowserplugin REUSE_FROM katepch)
endif()

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
