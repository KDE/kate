kate_add_plugin(katekonsoleplugin)
target_compile_definitions(katekonsoleplugin PRIVATE TRANSLATION_DOMAIN="katekonsoleplugin")
target_link_libraries(katekonsoleplugin PRIVATE KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextEditor)

target_sources(
  katekonsoleplugin 
  PRIVATE
    kateconsole.cpp 
    plugin.qrc
)

if (BUILD_PCH)
    target_precompile_headers(katekonsoleplugin REUSE_FROM katepch)
endif()
